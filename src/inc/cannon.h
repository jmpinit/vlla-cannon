#ifndef H_CANNON
#define H_CANNON

typedef struct color {
    int r, g, b;
} color;

typedef struct cannon {
    float angle;
    int score;
} cannon;

typedef struct missile {
    bool active;
    float x, y;
    float vx, vy;
    float radius;
} missile;

typedef struct alien {
    bool active;
    float x, y;
    float vx, vy;
    int points;
} alien;

#endif
