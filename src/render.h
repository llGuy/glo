#ifndef _RENDER_H_
#define _RENDER_H_

#include <stdint.h>

#include "glo.h"
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
  // char pad0[12];

  float time;
  float maxLazerTime;

  int controlledPlayer;
  int playerCount;

  /* Bullet trails */
  int bulletTrailCount;

  char pad[8];

  BulletTrajectory bulletTrails[MAX_BULLET_TRAILS];

  /* x,y coordinates; z=orient; w=scale */
  Vec4 wPlayerProp[MAX_PLAYER_COUNT];

} UniformData;

typedef struct RenderData {
  uint32_t shader;
  uint32_t uniformBuffer;
  uint32_t uniformBlockIdx;

  UniformData uniformData;
} RenderData;

RenderData *createRenderData(const struct DrawContext *ctx);

void render(
  const GloState *game, DrawContext *ctx,
  RenderData *renderData);

#endif
