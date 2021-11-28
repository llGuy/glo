#ifndef _NET_H_
#define _NET_H_

#include "glo.h"

#define MAX_COMMANDS 30

/* Time separating two command packets send */
#define COMMANDS_PACKET_INTERVAL 0.1f
#define SNAPSHOT_PACKET_INTERVAL 0.15f
#define MAIN_SOCKET_PORT_CLIENT 6000
#define MAIN_SOCKET_PORT_SERVER 5999
#define INVALID_CLIENT_ID 0x42

typedef struct Client {
  /* Index into the clients array */
  int id;

  /* Used to receive world state and send commands (or vice-versa) */
  int mainSocket;
  uint16_t mainSocketPort;

  uint32_t commandCount;
  GameCommands commandStack[MAX_COMMANDS];

  /* Predicted state after all the commands were executed */
  struct {
    Vec2 position;
    float orientation;
    float speed;
  } predicted;

  /* Used by the client program */
  uint32_t serverAddr;

  /* Used by the server program */
  uint32_t clientAddr;
  uint16_t clientPort;

  /* Time we last sent a commands packet */
  float lastCommandsSend;

  struct {
    uint8_t isConnected: 1;
    uint8_t predictionError: 1;
    uint8_t pad: 6;
  } flags;
} Client;

typedef struct Server {
  /* Main socket through which the server will send and receive messages */
  int mainSocket;

  /* Keeps track of all the active clients */
  int clientCount;
  Client clients[MAX_PLAYER_COUNT];

  /* Keeps track of the indices at which clients have been freed from above */
  int freeClientCount;

  /* Each bit corresponds to whether a client is active in the clients array */
  BitVector clientOccupation;

  /* Stack of free client indices */
  unsigned char freeClients[MAX_BULLET_TRAILS];

  /* Stack of new client indices to send in the next snapshot */
  uint32_t newClientCount;
  unsigned char newClientStack[MAX_PLAYER_COUNT];

  float lastSnapshotSend;
} Server;

enum PacketType {
  PT_DISCOVER, PT_CONNECT, PT_COMMANDS, PT_SNAPSHOT, PT_DISCONNECT
};

/* For protocol, see the readme */
typedef union PacketHeader {
  struct {
    uint32_t packetType: 4;
    uint32_t clientID: 5;
    /* May need additional information after? */
  };

  uint32_t bytes;
} PacketHeader;

/*****************************************************************************/
/*                                   Client                                  */
/*****************************************************************************/
Client createClient(uint16_t mainPort);
void waitForGameState(Client *c, GloState *game);
void pushGameCommands(Client *c, const GameCommands *commands);
void getServerAddress(Client *c);
void tickClient(Client *c, GloState *game);
void destroyClient(Client *);

/*****************************************************************************/
/*                                   Server                                  */
/*****************************************************************************/
Server createServer();
void tickServer(Server *s, GloState *game);
void destroyServer(Server *s);

#endif
