#include "math.h"

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
