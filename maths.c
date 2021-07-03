#include "maths.h"

inline void sub_vec3(vec3_t *u, vec3_t *v, vec3_t *r) {
  r->x = u->x - v->x;
  r->y = u->y - v->y;
  r->z = u->z - v->z;
}

inline float inner_prod_vec3(vec3_t *u, vec3_t *v) {
  return u->x * v->x + u->y * v->y + u->z * v->z;
}

inline void cross_prod_vec3(vec3_t *u, vec3_t *v, vec3_t *r) {
  r->x = u->y * v->z - u->z * v->y;
  r->y = u->z * v->x - u->x * v->z;
  r->z = u->x * v->y - u->y * v->x;
}

inline float len_vec3(vec3_t *v) {
  return sqrtf(inner_prod_vec3(v, v));
}

inline void norm_vec3(vec3_t *v, vec3_t *r) {
  float l = len_vec3(v);
  r->x = v->x / l;
  r->y = v->y / l;
  r->z = v->z / l;
}

inline void translate_mat4(vec3_t *v, float *m) {
}

void mult_mat4(float *m, float *n, float *r) {
  int i, j, k;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      r[i * 4 + j] = 0.0f;
      for (k = 0; k < 4; k++)
        r[i * 4 + j] += m[k * 4 + j] * n[i * 4 + k];
    }
  }
}

void load_identity(float *matrix) {
  int i, j;
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      matrix[i * 4 + j] = (float)(i == j);
}

void load_view(vec3_t *origin, vec3_t *eye, vec3_t *up, float *matrix) {
  vec3_t forward;
  sub_vec3(origin, eye, &forward);
  norm_vec3(&forward, &forward);

  vec3_t side;
  cross_prod_vec3(&forward, up, &side);
  norm_vec3(&side, &side);

  cross_prod_vec3(&side, &forward, up);

  matrix[0] = side.x;
  matrix[1] = up->x;
  matrix[2] = -forward.x;
  matrix[4] = side.y;
  matrix[5] = up->y;
  matrix[6] = -forward.y;
  matrix[8] = side.z;
  matrix[9] = up->z;
  matrix[10] = -forward.z;

  matrix[12] = side.x * eye->x + side.y * eye->y + side.z * eye->z;
  matrix[13] = up->x * eye->x + up->y * eye->y + up->z * eye->z;
  matrix[14] = forward.x * eye->x + forward.y * eye->y + forward.z * eye->z;
  matrix[15] = 1.0f;
}

void load_projection(float fov, float near, float far, float *matrix) {
  float t = 1.0f / tanf(fov / 2.0f),
    d = near - far;
  matrix[0] = t;
  matrix[5] = t;
  matrix[10] = (near + far) / d;
  matrix[11] = -1.0f;
  matrix[14] = 2.0f * near * far / d;
}
