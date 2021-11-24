#ifndef _STATE_H_
#define _STATE_H_

#include <stdint.h>

#include "math.h"
#include "bitv.h"

#define MAX_PLAYER_COUNT 20
#define MAX_BULLET_TRAILS 100
#define BASE_SPEED 5.0f

/* A player will have a radius of 1.0f meter. The grid will be of 8x8 squares */

typedef struct Player {
  Vec2 position;
  float orientation;
  float speed;
} Player;

typedef struct BulletTrajectory {
  Vec2 wStart;
  Vec2 wTrail[5];
  Vec2 wEnd;
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
} GameCommands;

typedef struct GloState {
  /* Player state which will need to be synced with the network */
  int playerCount;
  Player players[MAX_PLAYER_COUNT];

  /* Index of the player struct being controlled by this client */
  int controlled;

  /* Need to keep track of all the bullet trails and stuff */
  BitVector freeBulletTrails;
  int freeBulletTrailCount;
  BulletTrajectory bulletTrails[MAX_BULLET_TRAILS];
  int bulletTrailCount;
} GloState;

GloState *createGloState();
Player createPlayer(Vec2 position);
int createBulletTrail(GloState *game, Vec2 start, Vec2 direction);
void freeBulletTrail(GloState *game, int idx);

#endif
