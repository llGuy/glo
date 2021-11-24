#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "io.h"
#include "glo.h"
#include "render.h"

/* Initializes default game state - ready to join/create a game. */
GloState *createGloState() {
  GloState *state = (GloState *)malloc(sizeof(GloState));
  memset(state, 0, sizeof(GloState));

  state->freeBulletTrails = createBitvec(MAX_BULLET_TRAILS);

  return state;
}

/* Creates a player with default properties */
Player createPlayer(Vec2 position) {
  Player player = {
    .position = position,
    .orientation = 0.0f,
    .speed = BASE_SPEED
  };

  return player;
}

/* Needs to add to the trajectories array */
int createBulletTrail(GloState *game, Vec2 start, Vec2 direction) {
  if (game->freeBulletTrailCount) {
    
  }

  return 0;
}

/* Predict the state of the player */
static void predictState(GloState *gameState, GameCommands commands) {
  float dt = commands.dt;
  Player *me = &gameState->players[gameState->controlled];

  /* Update orientation */
  me->orientation = commands.newOrientation;

  /* Update position */
  if (commands.actions.moveUp) {
    me->position.y += dt*me->speed;
  }
  if (commands.actions.moveLeft) {
    me->position.x += -dt*me->speed;
  }
  if (commands.actions.moveDown) {
    me->position.y += -dt*me->speed;
  }
  if (commands.actions.moveRight) {
    me->position.x += dt*me->speed;
  }
  if (commands.actions.shoot)  {
    printf("Shoot\n");
  }
}

/* Entry point */
int main(int argc, char *argv[]) {
  DrawContext *drawContext = createDrawContext();
  RenderData *renderData = createRenderData(drawContext);
  GloState *gameState = createGloState();

  /* Create test player */
  gameState->controlled = 0;
  gameState->players[0] = createPlayer(vec2(0.0f, 0.0f));

  bool isRunning = true;

  while (isRunning) {
    GameCommands commands = translateIO(drawContext);
    predictState(gameState, commands);

    render(drawContext, gameState, renderData);
    tickDisplay(drawContext);

    isRunning = !isContextClosed(drawContext);
  }

  return 0;
}
