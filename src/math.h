#ifndef _MATH_H_
#define _MATH_H_

typedef union Vec2 {
  struct {float x, y;};
  float v[2];
} Vec2;

typedef union Vec4 {
  struct {float x, y, z, w;};
  float v[4];
} Vec4;

typedef union Mat4 {
  struct {Vec4 cols[4];};
  float v[16];
} Mat4;

typedef struct Rect2D {
  Vec2 min, size;
} Rect2D;

/*****************************************************************************/
/*                           Constructor functions                           */
/*****************************************************************************/

Vec2 vec2_orient(float orientation);
Vec2 vec2_add(Vec2 a, Vec2 b);
Vec2 vec2_mul(Vec2 a, float scale);
Vec2 vec2(float x, float y);
Vec4 vec4(float x, float y, float z, float w);

Mat4 identity();

/*****************************************************************************/
/*                               Math functions                              */
/*****************************************************************************/

/* Inverse orthographic projection - from normalized device to world coords */
Mat4 invOrtho(Vec2 wBase, float wWidth, float aspect);

#endif
