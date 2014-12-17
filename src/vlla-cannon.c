#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include "audio.h"
#include "vlla.h"

#define WIDTH 60
#define HEIGHT 32
#define NUM_PIXELS (WIDTH*HEIGHT)

void point(VLLA* vlla, int x, int y) {
    if(x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT) {
        vlla->pixels[y*WIDTH+x] = 0x00040404;
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

void line(VLLA* vlla, int x0, int y0, int x1, int y1) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    int D = 2*dy - dx;
    point(vlla, x0, y0);
    int y = y0;

    for(int x=x0+1; x <= x1; x++) {
        if(D > 0) {
            y = y+1;
            point(vlla, x, y);
            D = D + (2*dy-2*dx);
        } else {
            point(vlla, x, y);
            D = D + (2*dy);
        }
    }
}

int main(int argc, char *argv[]) {
    //init_fft();
    //init_audio();

    VLLA* vlla = vlla_init("/dev/ttyACM0", "/dev/ttyACM1");
    circle(vlla, 0, 16, 8);
    line(vlla, 0, 16, 14, 16);
    vlla_update(vlla);
}
