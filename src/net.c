#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "net.h"

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

static int s_send_to(
  int sock, struct sockaddr_in *address, char *buffer, uint32_t size) {
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

  /* Disable blocking */
  setSocketBlockingState(c.mainSocket, 0);

  return c;
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

  return s;
}

void tickServer() {
  
}

void destroyServer(Server *s) {
  shutdown(s->mainSocket, SHUT_RDWR);
}
