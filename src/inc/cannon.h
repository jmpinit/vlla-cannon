#ifndef H_CANNON
#define H_CANNON

typedef struct color {
    int r, g, b;
} color;

typedef struct cannon {
    float angle;
    int score;
} cannon;

typedef struct alien {
    int x, y;
    int points;
} alien;

#endif
