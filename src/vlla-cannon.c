#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include "cannon.h"
#include "audio.h"
#include "vlla.h"

#define forever for(;;)

#define M_PI 3.14159265358979323846

#define WIDTH 60
#define HEIGHT 32
#define NUM_PIXELS (WIDTH*HEIGHT)

uint8_t fft[FFT_SIZE];

color stroke = {0, 0, 0};

void color_set(color* c, int r, int g, int b) {
    c->r = r;
    c->g = g;
    c->b = b;
}

uint32_t color2vlla(color c) {
    return (c.r << 16) | (c.g << 8) | c.b;
}

void point(VLLA* vlla, int x, int y) {
    if(x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT) {
        vlla->pixels[y*WIDTH+x] = color2vlla(stroke);
    }
}

void circle(VLLA* vlla, int cx, int cy, int r) {
    for(int oy=-r; oy < r; oy++) {
        for(int ox=-r; ox < r; ox++) {
            float d = sqrt(pow(ox, 2) + pow(oy, 2));
            
            if(d < r) {
                int x = cx + ox;
                int y = cy + oy;

                point(vlla, x, y);
            }
        }
    }
}

void rect(VLLA* vlla, int x, int y, int w, int h) {
    for(int oy=0; oy < h; oy++) {
        for(int ox=0; ox < w; ox++) {
            point(vlla, x + ox, y + oy);
        }
    }
}

void clear(VLLA* vlla) {
    color_set(&stroke, 0, 0, 0);
    rect(vlla, 0, 0, WIDTH, HEIGHT);
}

void line(VLLA* vlla, int x0, int y0, int x1, int y1) {
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = (dx>dy ? dx : -dy)/2, e2;

    for(;;){
        point(vlla, x0, y0);
        if (x0==x1 && y0==y1) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

float snd_energy(uint8_t* fft, float bottom, float top) {
    int b = (int)(bottom * FFT_SIZE);
    int t = (int)(top * FFT_SIZE);

    float energy = 0;
    for(int i=b; i < t && i < FFT_SIZE; i++) {
        energy += fft[i];
    }

    return energy / (t-b);
}

float snd_main_freq(uint8_t* fft) {
    int max = fft[0];
    int max_i = 0;

    for(int i=0; i < FFT_SIZE; i++) {
        if(fft[i] > max) {
            max = fft[i];
            max_i = i;
        }
    }

    return (float) max_i / FFT_SIZE;
}

void gameloop() {
    VLLA* vlla = vlla_init("/dev/ttyACM0", "/dev/ttyACM1");

    cannon player = { 0, 0 };
    int length = 12;

    missile missiles[8];

    forever {
        // update
        fft_update();

        static float bottom = 0.25;
        static float top = 0.4;

        float freq = snd_main_freq(fft);
        float energy = snd_energy(fft, 0.1, 0.9);
        printf("%f\n", energy);

        if(freq > bottom && freq < top)
            player.angle = M_PI * (freq - bottom) / (top - bottom) - M_PI / 2.0;

        if(energy > 0.6) {
            missile* m = NULL;

            for(int i=0; i < 8 && m == NULL; i++) {
                if(!missiles[i].active)
                    m = &missiles[i];
            }

            if(m != NULL) {
                m->radius = 2;
                m->x = 0;
                m->y = 16;
                m->vx = cos(player.angle);
                m->vy = sin(player.angle);
                m->active = true;

                printf("missile fired!\n");
            }
        }

        for(int i=0; i < 8; i++) {
            if(missiles[i].active) {
                missiles[i].x += missiles[i].vx;
                missiles[i].y += missiles[i].vy;

                if(missiles[i].x < 0 || missiles[i].y < 0 || missiles[i].x >= WIDTH || missiles[i].y >= HEIGHT)
                    missiles[i].active = false;
            }
        }

        // render
        clear(vlla);
        color_set(&stroke, 5, 5, 5);

        circle(vlla, 0, 16, 8);
        line(vlla, 0, 16, length * cos(player.angle), 16 + length * sin(player.angle));

        for(int i=0; i < 8; i++) {
            if(missiles[i].active) {
                circle(vlla, (int)missiles[i].x, (int)missiles[i].y, missiles[i].radius);
            }
        }

        vlla_update(vlla);
    }
}

int main(int argc, char *argv[]) {
    audio_init(fft);
    gameloop();
}
