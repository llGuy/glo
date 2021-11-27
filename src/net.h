#ifndef _NET_H_
#define _NET_H_

#include "glo.h"

#define MAX_COMMANDS 10

#define MAIN_SOCKET_PORT_CLIENT 6000
#define MAIN_SOCKET_PORT_SERVER 5999

typedef struct Client {
  /* Used to receive world state and send commands (or vice-versa) */
  int mainSocket;
  uint16_t mainSocketPort;
  GameCommands commandStack[MAX_COMMANDS];
} Client;

typedef struct Server {
  int mainSocket;

  int clientCount;
  Client clients[MAX_PLAYER_COUNT];
} Server;

/*****************************************************************************/
/*                                   Client                                  */
/*****************************************************************************/
Client createClient(uint16_t mainPort);

/*****************************************************************************/
/*                                   Server                                  */
/*****************************************************************************/
Server createServer();
void destroyServer(Server *s);

#endif
