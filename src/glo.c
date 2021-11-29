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
    .speed = BASE_SPEED,
    .health = PLAYER_BASE_HEALTH
  };

  for (int i = 0; i < MAX_PLAYER_ACTIVE_TRAJECTORIES; ++i) {
    player.activeTrajectories[i] = INVALID_TRAJECTORY;
  }

  return player;
}

/* Creates a player and spawns at a random location */
Player *spawnPlayer(GloState *game, int idx) {
  Player *player = &game->players[idx];
  player->flags.isInitialized = 1;

  float radius = game->gridWidth / 2.0f;

  Vec2 mapStart = vec2(-game->gridBoxSize*radius, -game->gridBoxSize*radius);
  Vec2 mapEnd = vec2(game->gridBoxSize*radius, game->gridBoxSize*radius);

  player->position.x = randomf(mapStart.x, mapEnd.x);
  player->position.y = randomf(mapStart.y, mapEnd.y);
  player->orientation = randomf(0.0f, 6.3f);
  player->speed = BASE_SPEED;
  player->health = PLAYER_BASE_HEALTH;

  for (int i = 0; i < MAX_PLAYER_ACTIVE_TRAJECTORIES; ++i) {
    player->activeTrajectories[i] = INVALID_TRAJECTORY;
  }

  game->playerCount = MAX(game->playerCount, (idx+1));

  return player;
}

/* Needs to add to the trajectories array */
int createBulletTrail(
  GloState *game, Vec2 start, Vec2 end, float timeStart, int shooter) {
  int trajectoryIdx = -1;
  BulletTrajectory *trajectory = NULL;

  if (game->freeBulletTrailCount) {
    unsigned short freeBullet = game->freeBullets[--game->freeBulletTrailCount];
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
  trajectory->shooter = shooter;

  if (timeStart == -1.0f) {
    trajectory->timeStart = getTime();
  }
  else {
    trajectory->timeStart = timeStart;
  }

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

static void updatePlayerState(
  GloState *gameState, GameCommands commands,
  Player *player) {
  float dt = commands.dt;

  /* Update orientation */
  player->orientation = commands.newOrientation;

  Vec2 before = player->position;

  /* Update position */
  if (commands.actions.moveUp) {
    player->position.y += dt*player->speed;
  }
  if (commands.actions.moveLeft) {
    player->position.x += -dt*player->speed;
  }
  if (commands.actions.moveDown) {
    player->position.y += -dt*player->speed;
  }
  if (commands.actions.moveRight) {
    player->position.x += dt*player->speed;
  }
  if (commands.actions.shoot)  {
    int bulletIdx = createBulletTrail(
      gameState, player->position, commands.wShootTarget, -1.0f, 0);
  }

  player->position = keepInGridBounds(gameState, player->position);

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

/* Predict the state of the local game */
static void predictState(GloState *gameState, GameCommands commands) {
  float dt = commands.dt;
  Player *me = &gameState->players[gameState->controlled];

  updatePlayerState(gameState, commands, me);
}

static void interpolateState(GloState *gameState, float dt) {
  for (int i = 0; i < gameState->playerCount; ++i) {
    Player *p = &gameState->players[i];
    /* We want to interpolate state for all remote players */
    if (p->flags.isInitialized && i != gameState->controlled) {
      int b = p->snapshotStart, e = p->snapshotEnd;
      uint32_t snapshotCount = (e-b+MAX_PLAYER_SNAPSHOTS) % MAX_PLAYER_SNAPSHOTS;

      if (snapshotCount >= 3) {
        /* We need to make sure that there are enough snapshots so as to
           avoid halting */
        p->progress += dt/SNAPSHOT_PACKET_INTERVAL;
        if (p->progress >= 1.0f) {
          float newProgress = p->progress - floorf(p->progress);
          uint32_t skipCount = (uint32_t)(floorf(p->progress));

          p->progress = newProgress;

          b = (b+skipCount)%MAX_PLAYER_SNAPSHOTS;
        }

        PlayerSnapshot *s0 = &p->snapshots[b];
        PlayerSnapshot *s1 = &p->snapshots[(b+1)%MAX_PLAYER_SNAPSHOTS];

        p->position.x = lerp(s0->position.x, s1->position.x, p->progress);
        p->position.y = lerp(s0->position.y, s1->position.y, p->progress);
        p->orientation = lerp(s0->orientation, s1->orientation, p->progress);

        p->snapshotStart = b;
        p->snapshotEnd = e;
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
  waitForGameState(&client, gameState);

  bool isRunning = true;

  while (isRunning) {
    tickClient(&client, gameState);

    GameCommands commands = translateIO(drawContext);
    pushGameCommands(&client, &commands);
    predictState(gameState, commands);
    interpolateState(gameState, drawContext->dt);

    render(gameState, drawContext, renderData);
    tickDisplay(drawContext);

    isRunning = !isContextClosed(drawContext);
  }

  /* Send disconnect packet to the server */
  disconnectFromServer(&client);

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

static int checkBulletHit(BulletTrajectory *bullet, GloState *game) {
  for (int i = 0; i < game->playerCount; ++i) {
    Player *p = &game->players[i];
    if (p->flags.isInitialized) {
      Vec2 target = bullet->wEnd;
      Vec2 pPos = p->position;

      if (vec2_dist2(target, pPos) < 1.0f) {
        return i;
      }
    }
  }

  return -1;
}

/* The server is the program which authoritatively updates the game state */
static void tickGameState(Server *s, GloState *game) {
  for (int i = 0; i < s->clientCount; ++i) {
    Client *c = &s->clients[i];

    if (c->id != INVALID_CLIENT_ID) {
      Player *player = &game->players[c->id];

      Vec2 copy = player->position;

      /* Grind through those commands! */
      for (int command = 0; command < c->commandCount; ++command) {
        GameCommands commands = c->commandStack[command];
        if (commands.actions.moveUp) copy.y += commands.dt*player->speed;
        if (commands.actions.moveLeft) copy.x += -commands.dt*player->speed;
        if (commands.actions.moveDown) copy.y += -commands.dt*player->speed;
        if (commands.actions.moveRight) copy.x += commands.dt*player->speed;

        if (commands.actions.shoot) {
          int bulletIdx = createBulletTrail(
            game, player->position, commands.wShootTarget, -1.0f, c->id);
          game->newTrails[game->newTrailsCount++] = bulletIdx;

          int hitPlayer = checkBulletHit(&game->bulletTrails[bulletIdx], game);
          if (hitPlayer != -1) {
            Player *p = &game->players[hitPlayer];
            p->health -= 25;
            if (p->health <= 0) {
              spawnPlayer(game, hitPlayer);
            }
          }
        }
      }

      /* Does the predicted state match the actual state? */
      if (!eqf(copy.x, c->predicted.position.x, 0.0001f) ||
          !eqf(copy.y, c->predicted.position.y, 0.0001f)) {
        /* Prediction error - the client needs to fix this immediately */
        c->flags.predictionError = 1;
      }
      else {
        c->flags.predictionError = 0;
        player->position = copy;
        player->position = keepInGridBounds(game, player->position);
        player->orientation = c->predicted.orientation;
      }

      c->commandCount = 0;
    }
  }

  /* Predict which bullets to desintegrate */
  float currentTime = getTime();
  for (int i = 0; i < game->bulletTrailCount; ++i) {
    if (getBit(&game->bulletOccupation, i)) {
      if (currentTime - game->bulletTrails[i].timeStart >=
          MAX_LAZER_TIME+MAX_EXPLOSION_TIME) {
        freeBulletTrail(game, i);
      }
    }
  }
}

/* Server entry point */
int main(int argc, char *argv[]) {
  initializeGLFW();
  GloState *gameState = createGloState();

  signal(SIGINT, handleCtrlC);

  server = createServer();
  printf("Started server session\n");

  while (true) {
    tickServer(&server, gameState);
    tickGameState(&server, gameState);
  }

  return 0;
}
#endif
