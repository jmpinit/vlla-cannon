#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "kiss_fftr.h"
#include "audio.h"

pthread_t audio_t_id;
bool running = true;

snd_pcm_t *capture_handle;
snd_pcm_hw_params_t *hw_params;
unsigned int sampleRate;

uint8_t audiobuf[BUFFER_SIZE];
int16_t samplebuf[FFT_SIZE];
kiss_fft_cfg fft_cfg;
kiss_fft_cpx *fft_in;
kiss_fft_cpx *fft_out;
float log_pwr_fft[FFT_SIZE];
uint8_t* fft_shared_data;

void fft_init(uint8_t* fft_data);
void fft_shift(unsigned int amt);
void fft_update(void);

void fft_shift(unsigned int amt) {
    if(amt > FFT_SIZE) printf("\nFFT shifted too far.");

    int i;
    for(i = 0; i < FFT_SIZE - amt; i++) {
        fft_in[i].r = fft_in[i+amt].r;
        fft_in[i].i = fft_in[i+amt].i;
    }
}

void fft_update() {
    int i;

    kiss_fft_cpx pt;
    for(i = 0; i < FFT_SIZE/2; i++) {
        // normalize and convert to dBFS
        pt.r = fft_out[i].r / FFT_SIZE;
        pt.i = fft_out[i].i / FFT_SIZE;
        float pwr = pt.r * pt.r + pt.i * pt.i;

        log_pwr_fft[i] = 10.f * log10(pwr + 1.0e-20f);

        int v = OUTERSCALE * (YZERO + (int)(-log_pwr_fft[i] * SCALE));
        v = (v < 0)?      0       : v;
        v = (v > 0xff)?   0xff    : v;

        fft_shared_data[i] = v;
    }
}

void* audio_tick(void *arg) {
    int i;

    while(running) {
        // record some audio data
        int numFrames = BUFFER_SIZE/2;
        int sampleCount = snd_pcm_readi(capture_handle, audiobuf, numFrames);
        //int sampleCount = read(STDIN_FILENO, audiobuf, BUFFER_SIZE)/2;

        if(sampleCount < 0) {
            fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (sampleCount));
        } else {
            // downsample (TODO: FIR filter)
            for(i = 0; i < sampleCount; i += DOWNSAMPLE) {
                samplebuf[i/DOWNSAMPLE] = (audiobuf[i*2]) | (audiobuf[i*2+1] << 8);
            }

            int downsampleCount = sampleCount / DOWNSAMPLE;

            fft_shift(downsampleCount);

            for(i=0; i < downsampleCount; i += 2) {
                fft_in[FFT_SIZE-(downsampleCount + i)/2].r = (float)(samplebuf[i]) / 32767.f;
                fft_in[FFT_SIZE-(downsampleCount + i)/2].i = (float)(samplebuf[i+1]) / 32767.f;
            }

            kiss_fft(fft_cfg, fft_in, fft_out);
        }
    }
    return NULL;
}

void fft_init(uint8_t* fft_data) {
    fft_cfg = kiss_fft_alloc(FFT_SIZE, false, NULL, NULL);
    fft_in = (kiss_fft_cpx*)malloc(FFT_SIZE * sizeof(kiss_fft_cpx));
    fft_out = (kiss_fft_cpx*)malloc(FFT_SIZE * sizeof(kiss_fft_cpx));

    fft_shared_data = fft_data;

    if(fft_in == NULL || fft_out == NULL) {
        printf("Not enough memory.\n");
        exit(1);
    }
}

void audio_close() {
    snd_pcm_close(capture_handle);
}

void audio_init(uint8_t* fft_data) {
    int err;

    char* device = "mic";
    if ((err = snd_pcm_open (&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s (%s)\n", 
                device,
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    sampleRate = 44100;
    if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &sampleRate, 0)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
                snd_strerror (err));
        exit (1);
    }
    printf("sample rate is %d Hz.\n", sampleRate);

    if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (capture_handle)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    fft_init(fft_data);

    err = pthread_create(&(audio_t_id), NULL, &audio_tick, NULL);
    if (err != 0)
        printf("can't create audio thread :[%s]\n", strerror(err));
    else
        printf("audio thread created successfully\n");

    printf("created recording stream.\n");
    printf("FFT_SIZE=%d\n", FFT_SIZE);
}

