#ifndef H_AUDIO
#define H_AUDIO

#define BUFFER_SIZE 8000
#define DOWNSAMPLE 4
#define FFT_SIZE (BUFFER_SIZE / DOWNSAMPLE)

#define YZERO 65
#define SCALE -1.0f
#define OUTERSCALE 0.8f

void audio_init(uint8_t* fft_data);
void audio_close(void);

void fft_update(void);

#endif
