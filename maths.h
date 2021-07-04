#pragma once

#include <math.h>

#define PI (atanf(1.0f) * 4.0f)

typedef struct {
  float x, y;
} vec2_t;

typedef struct {
  float x, y, z;
} vec3_t;

void mult_mat4(float *, float *, float *);
void load_identity(float *);
void load_view(vec3_t *, vec3_t *, vec3_t *, float *);
void load_projection(float, float, float, float *);
void rotate_x(float, float *, float *);
void rotate_y(float, float *, float *);
void rotate_z(float, float *, float *);
