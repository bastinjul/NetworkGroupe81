#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <poll.h>
#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */

#include "wait_for_client.h"


int wait_for_client(int sfd, char* buffer){
  socklen_t addrlen;
  struct sockaddr_in6 src_addr;

  memset(&src_addr, 0, sizeof(src_addr));
  addrlen = sizeof(src_addr);
  int bytes_received = recvfrom(sfd, buffer, sizeof(buffer), MSG_PEEK, (struct sockaddr *)&src_addr, &addrlen);
  if (bytes_received == -1) {
      perror("recvfrom");
      return -1;
  }
  fprintf(stdout, "First packet reveived\n");

  if (connect(sfd, (struct sockaddr *)&src_addr, addrlen) == -1) {
      perror("connect");
      return -1;
  }
  fprintf(stdout, "Receiver connected to sender\n");

  return bytes_received;
}
