#ifndef _STATE_H_
#define _STATE_H_

#include "math.h"

#define MAX_PLAYER_COUNT 20

typedef struct Player {
  Vec2 position;
  float orientation;
} Player;

typedef struct GloState {
  /* Player state which will need to be synced with the network */
  int playerCount;
  Player players[MAX_PLAYER_COUNT];

  /* Need to keep track of all the bullet trails and stuff */
} GloState;

GloState *createGloState();

#endif
