#include <time.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "io.h"
#include "glo.h"
#include "net.h"
#include "render.h"

/* Initializes default game state - ready to join/create a game. */
GloState *createGloState() {
  GloState *state = (GloState *)malloc(sizeof(GloState));
  memset(state, 0, sizeof(GloState));

  state->bulletOccupation = createBitvec(MAX_BULLET_TRAILS);
  state->gridBoxSize = 6.0f;
  state->gridWidth = 8.0f;

  return state;
}

/* Creates a player with default properties */
Player createPlayer(Vec2 position) {
  Player player = {
    .position = position,
    .orientation = 0.0f,
    .speed = BASE_SPEED
  };

  for (int i = 0; i < MAX_PLAYER_ACTIVE_TRAJECTORIES; ++i) {
    player.activeTrajectories[i] = INVALID_TRAJECTORY;
  }

  return player;
}

/* Needs to add to the trajectories array */
int createBulletTrail(GloState *game, Vec2 start, Vec2 end) {
  int trajectoryIdx = -1;
  BulletTrajectory *trajectory = NULL;

  if (game->freeBulletTrailCount) {
    unsigned char freeBullet = game->freeBullets[--game->freeBulletTrailCount];
    trajectory = &game->bulletTrails[freeBullet];
    trajectoryIdx = (int)freeBullet;
  }
  else {
    trajectoryIdx = game->bulletTrailCount++;
    trajectory = &game->bulletTrails[trajectoryIdx];
  }

  setBit(&game->bulletOccupation, trajectoryIdx, 1);

  trajectory->wStart = start;
  trajectory->wEnd = end;
  trajectory->timeStart = getTime();

  return trajectoryIdx;
}

void freeBulletTrail(GloState *game, int idx) {
  setBit(&game->bulletOccupation, idx, 0);

  if (idx == game->bulletTrailCount - 1) {
    --game->bulletTrailCount;
  }
  else {
    game->freeBullets[game->freeBulletTrailCount++] = idx;
  }
}

static Vec2 keepInGridBounds(GloState *gameState, Vec2 wPos) {
  float radius = gameState->gridWidth / 2.0f;

  wPos.x = clamp(
    wPos.x, -radius*gameState->gridBoxSize, radius*gameState->gridBoxSize);
  wPos.y = clamp(
    wPos.y, -radius*gameState->gridBoxSize, radius*gameState->gridBoxSize);

  return wPos;
}

/* Predict the state of the local game */
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
    int bulletIdx = createBulletTrail(
      gameState, me->position, commands.wShootTarget);
  }

  me->position = keepInGridBounds(gameState, me->position);

  float currentTime = getTime();

  /* Predict which bullets to desintegrate */
  for (int i = 0; i < gameState->bulletTrailCount; ++i) {
    if (getBit(&gameState->bulletOccupation, i)) {
      if (currentTime - gameState->bulletTrails[i].timeStart >=
          MAX_LAZER_TIME+MAX_EXPLOSION_TIME) {
        freeBulletTrail(gameState, i);
      }
    }
  }
}

#ifdef BUILD_CLIENT
/*****************************************************************************/
/*                             Client entry point                            */
/*****************************************************************************/
static uint16_t getPort(int argc, char *argv[]) {
  if (argc > 1) {
    return atoi(argv[1]);
  }
  else {
    return MAIN_SOCKET_PORT_CLIENT;
  }
}

int main(int argc, char *argv[]) {
  DrawContext *drawContext = createDrawContext();
  RenderData *renderData = createRenderData(drawContext);
  GloState *gameState = createGloState();

  /* May add ability to change port */
  uint16_t port = getPort(argc, argv);
  Client client = createClient(port);

  /* Create test player */
  gameState->controlled = 0;
  gameState->players[0] = createPlayer(vec2(0.0f, 0.0f));
  gameState->players[1] = createPlayer(vec2(0.0f, 0.0f));
  gameState->playerCount = 2;

  bool isRunning = true;

  while (isRunning) {
    tickClient(&client);

    GameCommands commands = translateIO(drawContext);
    predictState(gameState, commands);

    render(gameState, drawContext, renderData);
    tickDisplay(drawContext);

    isRunning = !isContextClosed(drawContext);
  }

  return 0;
}
#else
/*****************************************************************************/
/*                             Server entry point                            */
/*****************************************************************************/
static Server server;

static void handleCtrlC(int signum) {
  destroyServer(&server);
  printf("Stopped server session\n");
  exit(signum);
}

/* Server entry point */
int main(int argc, char *argv[]) {
  signal(SIGINT, handleCtrlC);

  server = createServer();
  printf("Started server session\n");

  while (true) {
    tickServer(&server);
  }

  return 0;
}
#endif
