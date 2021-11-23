#ifndef _RENDER_H_
#define _RENDER_H_

#include <stdint.h>

#include "math.h"

typedef struct DrawContext DrawContext;
typedef struct GloState GloState;

typedef struct UniformData {
  Mat4 invOrtho;
  Vec2 wCirclePosition;
  float wCircleRadius;
} UniformData;

typedef struct RenderData {
  uint32_t shader;
  uint32_t uniformBuffer;
  uint32_t uniformBlockIdx;

  UniformData uniformData;
} RenderData;

RenderData *createRenderData(const struct DrawContext *ctx);

void render(
  const DrawContext *ctx, const GloState *game,
  RenderData *renderData);

#endif
