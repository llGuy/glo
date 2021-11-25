#include <math.h>

#include "math.h"

Vec2 vec2_orient(float orientation) {
  Vec2 v = {
    .x = cosf(orientation), .y = sinf(orientation)
  };

  return v;
}

Vec2 vec2_add(Vec2 a, Vec2 b) {
  Vec2 v = {
    .x = a.x + b.x, .y = a.y + b.y
  };

  return v;
}

Vec2 vec2_mul(Vec2 a, float scale) {
  Vec2 v = {
    .x = a.x * scale, .y = a.y * scale
  };

  return v;
}

Vec4 vec4(float x, float y, float z, float w) {
  Vec4 v = {
    .x = x, .y = y, .z = z, .w = w
  };

  return v;
}

Vec2 vec2(float x, float y) {
  Vec2 v = {
    .x = x, .y = y
  };

  return v;
}

Mat4 identity() {
  Mat4 mat = {};
  mat.cols[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);
  mat.cols[1] = vec4(0.0f, 1.0f, 0.0f, 0.0f);
  mat.cols[2] = vec4(0.0f, 0.0f, 1.0f, 0.0f);
  mat.cols[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
  return mat;
}

Vec4 mat4_mul_vec4(const Mat4 *m, const Vec4 *v) {
  Vec4 res = {
    .x = m->cols[0].v[0]*v->x+m->cols[1].v[0]*v->y+m->cols[2].v[0]*v->z+m->cols[3].v[0]*v->w,
    .y = m->cols[0].v[1]*v->x+m->cols[1].v[1]*v->y+m->cols[2].v[1]*v->z+m->cols[3].v[1]*v->w,
    .z = m->cols[0].v[2]*v->x+m->cols[1].v[2]*v->y+m->cols[2].v[2]*v->z+m->cols[3].v[2]*v->w,
    .w = m->cols[0].v[3]*v->x+m->cols[1].v[3]*v->y+m->cols[2].v[3]*v->z+m->cols[3].v[3]*v->w,
  };

  return res;
}

Mat4 invOrtho(Vec2 wBase, float wWidth, float aspect) {
  float wHeight = wWidth / aspect;

  Mat4 projection = identity();

  /* Scale appropriately */
  projection.cols[0].v[0] = wWidth/2.0f;
  projection.cols[1].v[1] = wHeight/2.0f;
  /* Translate appropriately */
  projection.cols[3].v[0] = wBase.x + (wWidth / 2.0f);
  projection.cols[3].v[1] = wBase.y + (wHeight / 2.0f);

  return projection;
}
