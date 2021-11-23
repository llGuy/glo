#ifndef _RENDER_H_
#define _RENDER_H_

#include <stdint.h>

#include "math.h"

typedef struct DrawContext DrawContext;
typedef struct GloState GloState;

typedef struct UniformData {

  /* Inverse orthographic projection to get from pixel space to world space. */
  Mat4 invOrtho;

  /* Start and end coordinates of the map to draw the grid. */
  Vec2 wMapStart;
  Vec2 wMapEnd;
  float wGridScale;

  /* Will see some padding due to layout std140 */
  char pad[4];

  /* Some test object. */
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
