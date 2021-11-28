#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "io.h"
#include "net.h"
#include "math.h"

#define MSG_BUFFER_SIZE 1000
static uint8_t msgBuffer[MSG_BUFFER_SIZE];

/*****************************************************************************/
/*                                Socket stuff                               */
/*****************************************************************************/
static void bindSocket(int sock, struct sockaddr_in *addr) {
  if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
    fprintf(stderr, "Failed to bind socket to port - incrementing\n");
    addr->sin_port++;
    bindSocket(sock, addr);
  }
}

static void setSocketBlockingState(int sock, int enabled) {
  int32_t flags = fcntl(sock, F_GETFL, 0);
    
  if (flags == -1) {
    // Error
  }

  if (enabled) {
    // Enable blocking
    flags &= ~O_NONBLOCK;
  }
  else {
    // Enable non-blocking
    flags |= O_NONBLOCK;
  }

  fcntl(sock, F_SETFL, flags);
}

static int32_t receivePacket(
  int sock, char *buffer, uint32_t size, struct sockaddr_in *dst) {
  struct sockaddr_in fromAddress = {};
  socklen_t fromSize = sizeof(fromAddress);

  int32_t bytesReceived = recvfrom(
    sock, buffer, size, 0, (struct sockaddr *)&fromAddress, &fromSize);

  if (bytesReceived < 0) {
    return 0;
  }
  else {
    buffer[bytesReceived] = 0;
  }

  *dst = fromAddress;

  return bytesReceived;
}

static int sendPacket(
  int sock, struct sockaddr_in *address, const char *buffer, uint32_t size) {
  int32_t sendto_ret = sendto(
    sock, buffer, size, 0, (struct sockaddr *)address, sizeof(*address));

  if (sendto_ret < 0) {
    // Error
    fprintf(stderr, "Failed to call sendto: %d\n", errno);

    return 0;
  }
  else {
    return 1;
  }
}

static uint32_t strToIpv4(
  const char *name, uint32_t port, int32_t protocol) {
  struct addrinfo hints = {}, *addresses;

  hints.ai_family = AF_INET;

  switch (protocol) {
  case IPPROTO_UDP: {
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    break;
  }
        
  case IPPROTO_TCP: {
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    break;
  }
  }

  char portStr[16] = {};
  sprintf(portStr, "%d", port);

  int32_t err = getaddrinfo(name, portStr, &hints, &addresses);

  if (err != 0) {
    fprintf(stderr, "%s: %s\n", name, gai_strerror(err));
  }

  // (This is a really bad way)
  // Find way to get correct address
  if (addresses) {
    for (struct addrinfo *addr = addresses; addr != NULL; addr = addr->ai_next) {
      if (addr->ai_family == AF_INET) {
        struct sockaddr_in *addrIn = (struct sockaddr_in *)addr->ai_addr;
        uint32_t address = addrIn->sin_addr.s_addr;
        freeaddrinfo(addresses);
        return address;
      }
    }

    fprintf(stderr, "Couldn't find address: %s\n", name);

    return 0;
  }
  else {
    return 0;
  }
}

/*****************************************************************************/
/*                               Data transfer                               */
/*****************************************************************************/
void serializeByte(unsigned char b, uint8_t *buffer, uint32_t *dstCounter) {
  buffer[(*dstCounter)++] = b;
}

unsigned char deserializeByte(uint8_t *buffer, uint32_t *dstCounter) {
  return buffer[(*dstCounter)++];
}

void serializeFloat32(float f32, uint8_t *buffer, uint32_t *dstCounter) {
  buffer = &buffer[*dstCounter];
  *dstCounter += 4;
#if defined (__i386) || defined (__x86_64__) || defined (_M_IX86) || defined(_M_X64)
  *(float *)buffer = f32;
#else
  uint32_t *f = (uint32_t *)&f32;
  *(buffer++) = (uint8_t)*f;
  *(buffer++) = (uint8_t)(*f >> 8);
  *(buffer++) = (uint8_t)(*f >> 16);
  *(buffer++) = (uint8_t)(*f >> 24);
#endif
}

float deserializeFloat32(uint8_t *buffer, uint32_t *dstCounter) {
  buffer = &buffer[*dstCounter];
  *dstCounter += 4;
#if defined (__i386) || defined (__x86_64__) || defined (_M_IX86) || defined(_M_X64)
  return(*(float *)buffer);
#else
  uint32_t ret = 0;
  ret += (*(buffer++));
  ret += ((uint32_t)(*(buffer++))) << 8;
  ret += ((uint32_t)(*(buffer++))) << 16;
  ret += ((uint32_t)(*(buffer++))) << 24;
    
  return(*(float *)(&ret));
#endif
}

void serializeUint32(uint32_t u32, uint8_t *buffer, uint32_t *dstCounter) {
  buffer = &buffer[*dstCounter];
  *dstCounter += 4;
#if defined (__i386) || defined (__x86_64__) || defined (_M_IX86) || defined(_M_X64)
  *(uint32_t *)buffer = u32;
#else
  *(buffer++) = (uint8_t)u32;
  *(buffer++) = (uint8_t)(u32 >> 8);
  *(buffer++) = (uint8_t)(u32 >> 16);
  *(buffer++) = (uint8_t)(u32 >> 24);
#endif
}

uint32_t deserializeUint32(uint8_t *buffer, uint32_t *dstCounter) {
  buffer = &buffer[*dstCounter];
  *dstCounter += 4;
#if defined (__i386) || defined (__x86_64__) || defined (_M_IX86) || defined(_M_X64)
  return(*(uint32_t *)buffer);
#else
  uint32_t ret = 0;
  ret += (*(buffer++));
  ret += ((uint32_t)(*(buffer++))) << 8;
  ret += ((uint32_t)(*(buffer++))) << 16;
  ret += ((uint32_t)(*(buffer++))) << 24;
  return(ret);
#endif
}

static uint32_t serializePacketHeader(Client *c, int packetType) {
  PacketHeader header = {
    .packetType = packetType,
    .clientID = c->id
  };

  uint32_t msgPtr = 0;
  serializeUint32(header.bytes, msgBuffer, &msgPtr);

  return msgPtr;
}

static uint32_t deserializePacketHeader(PacketHeader *header) {
  uint32_t msgPtr = 0;
  header->bytes = deserializeUint32(msgBuffer, &msgPtr);

  return msgPtr;
}

static uint32_t serializeCommands(Client *c, uint32_t *msgPtr) {
  serializeFloat32(c->predicted.position.x, msgBuffer, msgPtr);
  serializeFloat32(c->predicted.position.y, msgBuffer, msgPtr);
  serializeFloat32(c->predicted.orientation, msgBuffer, msgPtr);
  serializeFloat32(c->predicted.speed, msgBuffer, msgPtr);

  /* Command count */
  serializeUint32(c->commandCount, msgBuffer, msgPtr);
  /* Commands[] */
  for (int i = 0; i < c->commandCount; ++i) {
    GameCommands *command = &c->commandStack[i];
    serializeUint32(command->actions.bytes, msgBuffer, msgPtr);
    serializeFloat32(command->newOrientation, msgBuffer, msgPtr);
    serializeFloat32(command->dt, msgBuffer, msgPtr);
    serializeFloat32(command->wShootTarget.x, msgBuffer, msgPtr);
    serializeFloat32(command->wShootTarget.y, msgBuffer, msgPtr);
  }

  c->commandCount = 0;

  /* Return the size of this packet */
  return *msgPtr;
}

/* This will add the commands to the client's command stack */
static uint32_t deserializeCommands(Client *c, uint32_t *msgPtr) {
  c->predicted.position.x = deserializeFloat32(msgBuffer, msgPtr);
  c->predicted.position.y = deserializeFloat32(msgBuffer, msgPtr);
  c->predicted.orientation = deserializeFloat32(msgBuffer, msgPtr);
  c->predicted.speed = deserializeFloat32(msgBuffer, msgPtr);

  /* Command count */
  uint32_t commandCount = deserializeUint32(msgBuffer, msgPtr);
  // c->commandCount = MIN(c->commandCount, MAX_COMMANDS);
  /* Commands[] */
  for (int i = 0; i < commandCount && c->commandCount < MAX_COMMANDS; ++i) {
    GameCommands *command = &c->commandStack[c->commandCount];
    command->actions.bytes = deserializeUint32(msgBuffer, msgPtr);
    command->newOrientation = deserializeFloat32(msgBuffer, msgPtr);
    command->dt = deserializeFloat32(msgBuffer, msgPtr);
    command->wShootTarget.x = deserializeFloat32(msgBuffer, msgPtr);
    command->wShootTarget.y = deserializeFloat32(msgBuffer, msgPtr);
    c->commandCount++;
  }

  /* Return the size of this packet */
  return *msgPtr;
}

static uint32_t serializeConnect(
  Server *s, Client *c, GloState *game, uint32_t *msgPtr) {
  /* Client ID */
  serializeUint32((uint32_t)c->id, msgBuffer, msgPtr);
  /* Player count */
  serializeUint32((uint32_t)s->clientCount, msgBuffer, msgPtr);
  /* Players[] */
  for (int i = 0; i < s->clientCount; ++i) {
    Client *currentClient = &s->clients[i];

    serializeUint32((uint32_t)currentClient->id, msgBuffer, msgPtr);

    if (currentClient->id == INVALID_CLIENT_ID) {
      serializeFloat32(0.0f, msgBuffer, msgPtr);
      serializeFloat32(0.0f, msgBuffer, msgPtr);
      serializeFloat32(0.0f, msgBuffer, msgPtr);
      serializeFloat32(0.0f, msgBuffer, msgPtr);
    }
    else {
      Player *player = &game->players[currentClient->id];
      serializeFloat32(player->position.x, msgBuffer, msgPtr);
      serializeFloat32(player->position.y, msgBuffer, msgPtr);
      serializeFloat32(player->orientation, msgBuffer, msgPtr);
      serializeFloat32(player->speed, msgBuffer, msgPtr);
    }
  }

  /* Serialize current trajectories as well */
  /* Client will have to calculate the starting time of the trajectories */

  /* Return the size of this packet */
  return *msgPtr;
}

/* This will add the commands to the client's command stack */
static uint32_t deserializeConnect(Client *c, GloState *game, uint32_t *msgPtr) {
  /* Client ID */
  game->controlled = c->id = (int)deserializeUint32(msgBuffer, msgPtr);
  /* Player count */
  game->playerCount = deserializeUint32(msgBuffer, msgPtr);
  /* Players[] */
  for (int i = 0; i < game->playerCount; ++i) {
    Player *currentPlayer = &game->players[i];

    int currentID = (int)deserializeUint32(msgBuffer, msgPtr);

    if (currentID == INVALID_CLIENT_ID) {
      deserializeFloat32(msgBuffer, msgPtr);
      deserializeFloat32(msgBuffer, msgPtr);
      deserializeFloat32(msgBuffer, msgPtr);
      deserializeFloat32(msgBuffer, msgPtr);
    }
    else {
      Player *player = &game->players[currentID];
      player->position.x = deserializeFloat32(msgBuffer, msgPtr);
      player->position.y = deserializeFloat32(msgBuffer, msgPtr);
      player->orientation = deserializeFloat32(msgBuffer, msgPtr);
      player->speed = deserializeFloat32(msgBuffer, msgPtr);
    }
  }

  /* Return the size of this packet */
  return *msgPtr;
}

static uint32_t serializeSnapshot(
  Server *s, GloState *game, uint32_t *msgPtr) {
  /* Place holder value to be filled in after function is called */
  serializeByte((unsigned char)0, msgBuffer, msgPtr);

  /* Players[] */
  for (int i = 0; i < s->clientCount; ++i) {
    Client *currentClient = &s->clients[i];

    serializeUint32((uint32_t)currentClient->id, msgBuffer, msgPtr);

    if (currentClient->id == INVALID_CLIENT_ID) {
      serializeFloat32(0.0f, msgBuffer, msgPtr);
      serializeFloat32(0.0f, msgBuffer, msgPtr);
      serializeFloat32(0.0f, msgBuffer, msgPtr);
      serializeFloat32(0.0f, msgBuffer, msgPtr);
    }
    else {
      Player *player = &game->players[currentClient->id];
      serializeFloat32(player->position.x, msgBuffer, msgPtr);
      serializeFloat32(player->position.y, msgBuffer, msgPtr);
      serializeFloat32(player->orientation, msgBuffer, msgPtr);
      serializeFloat32(player->speed, msgBuffer, msgPtr);
    }
  }

  /* Serialize current trajectories as well */
  /* Client will have to calculate the starting time of the trajectories */

  /* Return the size of this packet */
  return *msgPtr;
}

static uint32_t deserializeSnapshot(
  Client *c, GloState *game, uint32_t *msgPtr) {
  /* Place holder value to be filled in after function is called */
  c->flags.predictionError = deserializeByte(msgBuffer, msgPtr);

  /* Players[] */
  for (int i = 0; i < game->playerCount; ++i) {
    Player *currentPlayer = &game->players[i];

    int currentID = (int)deserializeUint32(msgBuffer, msgPtr);

    if (currentID == INVALID_CLIENT_ID) {
      deserializeFloat32(msgBuffer, msgPtr);
      deserializeFloat32(msgBuffer, msgPtr);
      deserializeFloat32(msgBuffer, msgPtr);
      deserializeFloat32(msgBuffer, msgPtr);
    }
    else {
      Player *player = &game->players[currentID];
      if (currentID != game->controlled) {
        player->position.x = deserializeFloat32(msgBuffer, msgPtr);
        player->position.y = deserializeFloat32(msgBuffer, msgPtr);
        player->orientation = deserializeFloat32(msgBuffer, msgPtr);
        player->speed = deserializeFloat32(msgBuffer, msgPtr);
      }
      else if (c->flags.predictionError) {
        printf("Made prediction error\n");
        player->position.x = deserializeFloat32(msgBuffer, msgPtr);
        player->position.y = deserializeFloat32(msgBuffer, msgPtr);
        player->orientation = deserializeFloat32(msgBuffer, msgPtr);
        player->speed = deserializeFloat32(msgBuffer, msgPtr);

        /* Reset the command stack */
        c->commandCount = 0;
      }
    }
  }

  /* Return the size of this packet */
  return *msgPtr;
}

static void sendPacketToServer(Client *c, uint8_t *msg, uint32_t size) {
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = MAIN_SOCKET_PORT_SERVER;
  addr.sin_addr.s_addr = c->serverAddr;
  sendPacket(c->mainSocket, &addr, (char *)msg, size);
}

/*****************************************************************************/
/*                                   Client                                  */
/*****************************************************************************/
Client createClient(uint16_t mainPort) {
  Client c = {
    .mainSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP),
    .commandCount = 0,
    .lastCommandsSend = 0.0f
  };

  if (c.mainSocket < 0) {
    fprintf(stderr, "Failed to create client socket: %d\n", errno);
    exit(-1);
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = mainPort;
  addr.sin_addr.s_addr = INADDR_ANY;

  /* Bind socket */
  bindSocket(c.mainSocket, &addr);
  c.mainSocketPort = addr.sin_port;
  printf("Bound to port %d\n", (int)addr.sin_port);

  /* Disable blocking */
  setSocketBlockingState(c.mainSocket, 0);

  getServerAddress(&c);

  return c;
}

void waitForGameState(Client *c, GloState *game) {
  /* Send a connection request to server */
  uint32_t msgSize = 0;
  PacketHeader header = {.packetType = PT_DISCOVER};
  serializeUint32(header.bytes, msgBuffer, &msgSize);
  sendPacketToServer(c, msgBuffer, msgSize);

  for (int recvCount = 0; recvCount < 10; ++recvCount) {
    usleep(5);

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = MAIN_SOCKET_PORT_SERVER;
    addr.sin_addr.s_addr = c->serverAddr;
    int size = receivePacket(
      c->mainSocket, (char *)msgBuffer, MSG_BUFFER_SIZE, &addr);

    if (size > 0) {
      printf("Received game state: ready to play!\n");

      c->flags.isConnected = 1;

      PacketHeader header = {};
      uint32_t msgPtr = deserializePacketHeader(&header);

      assert(header.packetType == PT_CONNECT);
      deserializeConnect(c, game, &msgPtr);
    }
  }
}

void pushGameCommands(Client *c, const GameCommands *commands) {
  if (c->commandCount < MAX_COMMANDS) {
    c->commandStack[c->commandCount++] = *commands;
  }
}

void getServerAddress(Client *c) {
  /* For testing purposes */
  c->serverAddr = strToIpv4("127.0.0.1", MAIN_SOCKET_PORT_SERVER, IPPROTO_UDP);
}

void tickClient(Client *c, GloState *game) {
  if (c->flags.isConnected) {
    /* Receive all the packets the server sent */
    for (int i = 0; i < 5 /* May increase in future */; ++i) {
      struct sockaddr_in addr = {};
      int size = receivePacket(
        c->mainSocket, (char *)msgBuffer, MSG_BUFFER_SIZE, &addr);

      if (size > 0) {
        PacketHeader header = {};
        uint32_t msgPtr = deserializePacketHeader(&header);

        switch (header.packetType) {
        case PT_SNAPSHOT: {
          printf("Received snapshot\n");
          deserializeSnapshot(c, game, &msgPtr);
        } break;

          /* Other stuff... */
        }
      }
    }

    float currentTime = getTime();
    if (currentTime - c->lastCommandsSend > COMMANDS_PACKET_INTERVAL) {
      c->lastCommandsSend = currentTime;

      uint32_t msgPtr = serializePacketHeader(c, PT_COMMANDS);

      /* Set the client's predicted state for serialization purporses */
      Player *p = &game->players[c->id];
      c->predicted.position = p->position;
      c->predicted.orientation = p->orientation;
      c->predicted.speed = p->speed;

      /* Flush all the commands in the command stack and send the packet */
      uint32_t byteCount = serializeCommands(c, &msgPtr);
      if (!gSimulatePacketLoss) {
        sendPacketToServer(c, msgBuffer, byteCount);
      }
      else {
        printf("PACKET LOSS\n");
      }
    }
  }
}

void destroyClient(Client *c) {
  
}

/*****************************************************************************/
/*                                   Server                                  */
/*****************************************************************************/
static int addClient(Server *server) {
  int clientIdx = -1;
  Client *client = NULL;

  if (server->freeClientCount) {
    unsigned char freeClient = server->freeClients[--server->freeClientCount];
    client = &server->clients[freeClient];
    clientIdx = (int)freeClient;
  }
  else {
    clientIdx = server->clientCount++;
    client = &server->clients[clientIdx];
  }

  setBit(&server->clientOccupation, clientIdx, 1);

  memset(client, 0, sizeof(Client));

  return clientIdx;
}

static void freeClient(Server *server, int idx) {
  server->clients[idx].flags.isConnected = 0;
  server->clients[idx].id = INVALID_CLIENT_ID;

  setBit(&server->clientOccupation, idx, 0);

  if (idx == server->clientCount - 1) {
    --server->clientCount;
  }
  else {
    server->freeClients[server->freeClientCount++] = idx;
  }
}

Server createServer() {
  Server s = {
    .mainSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP),
  };

  if (s.mainSocket < 0) {
    fprintf(stderr, "Failed to create server socket: %d\n", errno);
    exit(-1);
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = MAIN_SOCKET_PORT_SERVER;
  addr.sin_addr.s_addr = INADDR_ANY;

  /* Bind socket */
  bindSocket(s.mainSocket, &addr);

  /* Disable blocking */
  setSocketBlockingState(s.mainSocket, 0);

  s.clientOccupation = createBitvec(MAX_PLAYER_COUNT);
  s.lastSnapshotSend = 0.0f;

  return s;
}

static void sendSnapshotToClient(Server *s, Client *c, uint32_t size) {
  uint32_t headerSize = sizeof(PacketHeader);
  /* Just change the byte saying that prediction correction is needed */
  serializeByte(
    (unsigned char)c->flags.predictionError,
    msgBuffer, &headerSize);

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = c->clientPort;
  addr.sin_addr.s_addr = c->clientAddr;

  sendPacket(s->mainSocket, &addr, (char *)msgBuffer, size);
}

void tickServer(Server *server, GloState *game) {
  /* Send out the game state to all clients */
  float currentTime = getTime();
  if (currentTime - server->lastSnapshotSend >= SNAPSHOT_PACKET_INTERVAL) {
    printf("Sent snapshot to clients\n");
    server->lastSnapshotSend = currentTime;

    uint32_t msgPtr = serializePacketHeader(&server->clients[0], PT_SNAPSHOT);
    serializeSnapshot(server, game, &msgPtr);

    for (int i = 0; i < server->clientCount; ++i) {
      Client *c = &server->clients[i];

      if (c->id != INVALID_CLIENT_ID) {
        /* Send out game state */
        sendSnapshotToClient(server, c, msgPtr);
      }
    }
  }

  /* Receive packets from the clients */
  for (int i = 0; i < MAX_PLAYER_COUNT; ++i) {
    struct sockaddr_in addr;

    int32_t byteCount = receivePacket(
      server->mainSocket, (char *)msgBuffer, MSG_BUFFER_SIZE, &addr);

    if (byteCount > 0) {
      PacketHeader header = {};
      uint32_t msgCounter = deserializePacketHeader(&header);

      switch (header.packetType) {
      case PT_DISCOVER: {
        /* Create a new client and send a handshake back */
        int id = addClient(server);

        /* Initialize client information */
        Client *c = &server->clients[id];
        c->id = id;
        c->clientAddr = addr.sin_addr.s_addr;
        c->clientPort = addr.sin_port;
        c->flags.isConnected = 1;

        /* Initialize predicted data */
        Player *p = spawnPlayer(game, id);
        c->predicted.position = p->position;
        c->predicted.orientation = p->orientation;
        c->predicted.speed = p->speed;

        /* Create connect packet */
        uint32_t msgPtr = serializePacketHeader(c, PT_CONNECT);
        uint32_t size = serializeConnect(server, c, game, &msgPtr);

        /* Send back to client that just sent this message */
        sendPacket(server->mainSocket, &addr, (char *)msgBuffer, size);

        printf("Received discover packet - sent connection packet\n");
      } break;

      case PT_COMMANDS: {
        int clientID = header.clientID;

        uint32_t size = deserializeCommands(
          &server->clients[clientID], &msgCounter);

        printf("Received commands packet from %d\n", clientID);
      } break;
      }
    }
  }

  /* Sleep for 5 milliseconds */
  usleep(5);
}

void destroyServer(Server *s) {
  shutdown(s->mainSocket, SHUT_RDWR);
}
