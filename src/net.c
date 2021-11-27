#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "net.h"

#define MSG_BUFFER_SIZE 1000
static char msgBuffer[MSG_BUFFER_SIZE];

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
/*                                   Client                                  */
/*****************************************************************************/
Client createClient(uint16_t mainPort) {
  Client c = {
    .mainSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP),
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

  return c;
}

void getServerAddress(Client *c) {
  /* For testing purposes */
  c->serverAddr = strToIpv4("127.0.0.1", MAIN_SOCKET_PORT_SERVER, IPPROTO_UDP);
}

void tickClient(Client *c) {
  const char *testStr = "Hello Server!";

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = MAIN_SOCKET_PORT_SERVER;
  addr.sin_addr.s_addr = c->serverAddr;
  sendPacket(c->mainSocket, &addr, testStr, strlen(testStr));
}

void destroyClient(Client *c) {
  
}

/*****************************************************************************/
/*                                   Server                                  */
/*****************************************************************************/
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

  return s;
}

void tickServer(Server *server) {
  /* Call receive 10 times */
  for (int i = 0; i < 10; ++i) {
    struct sockaddr_in addr;

    int32_t byteCount = receivePacket(
      server->mainSocket, msgBuffer, MSG_BUFFER_SIZE, &addr);

    if (byteCount > 0) {
      printf("Received %d bytes: %s\n", byteCount, msgBuffer);
    }
  }

  /* Sleep for 5 milliseconds */
  usleep(5);
}

void destroyServer(Server *s) {
  shutdown(s->mainSocket, SHUT_RDWR);
}
