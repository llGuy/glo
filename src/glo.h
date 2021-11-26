#ifndef _STATE_H_
#define _STATE_H_

#include <stdint.h>

#include "math.h"
#include "bitv.h"

#define MAX_PLAYER_COUNT 20
#define MAX_BULLET_TRAILS 100
#define MAX_PLAYER_ACTIVE_TRAJECTORIES 4
#define BASE_SPEED 5.0f
#define INVALID_TRAJECTORY (-1)
#define MAX_LAZER_TIME 0.2f
#define MAX_EXPLOSION_TIME 0.15f

/* A player will have a radius of 1.0f meter. The grid will be of 8x8 squares */

typedef struct Player {
  Vec2 position;
  float orientation;
  float speed;
  char activeTrajectories[MAX_PLAYER_ACTIVE_TRAJECTORIES];
} Player;

typedef struct BulletTrajectory {
  Vec2 wStart;
  Vec2 wTrail[1];
  Vec2 wEnd;
  float timeStart;

  char pad[4];
} BulletTrajectory;

typedef struct GameCommands {
  struct {
    uint32_t moveUp : 1;
    uint32_t moveLeft : 1;
    uint32_t moveDown : 1;
    uint32_t moveRight : 1;
    uint32_t shoot : 1;
    /* Add other commands and powers in the future like dash or whatever. */
    uint32_t pad : 27;
  } actions;

  /* Orientation is calculated by diffing cursor pos with center */
  float newOrientation;

  /* Frame time in seconds */
  float dt;

  /* Where the user clicks is where the bullet will go */
  Vec2 wShootTarget;
} GameCommands;

typedef struct GloState {
  /* Player state which will need to be synced with the network */
  int playerCount;
  Player players[MAX_PLAYER_COUNT];

  /* Index of the player struct being controlled by this client */
  int controlled;

  /* Need to keep track of all the bullet trails and stuff */
  int freeBulletTrailCount;
  BitVector bulletOccupation;
  unsigned char freeBullets[MAX_BULLET_TRAILS];
  BulletTrajectory bulletTrails[MAX_BULLET_TRAILS];
  int bulletTrailCount;

  float gridBoxSize;
  /* In grid boxes */
  float gridWidth;
} GloState;

GloState *createGloState();
Player createPlayer(Vec2 position);
int createBulletTrail(GloState *game, Vec2 start, Vec2 end);
void freeBulletTrail(GloState *game, int idx);

#endif
