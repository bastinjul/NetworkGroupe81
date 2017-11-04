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

#define MAX_BUFFER_SIZE 1024
/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd){
  char buffer[MAX_BUFFER_SIZE];
  socklen_t addrlen;
  struct sockaddr_in6 src_addr;
  memset(&src_addr, 0, sizeof(src_addr));
  addrlen = sizeof(src_addr);
  ssize_t bytes_received = recvfrom(sfd, buffer, sizeof(buffer), MSG_PEEK, (struct sockaddr *)&src_addr, &addrlen);
  if (bytes_received == -1) {
      fprintf(stderr, "Error : recvfrom()\n");
      return -1;
  }

  if (connect(sfd, (struct sockaddr *)&src_addr, addrlen) == -1) {
      fprintf(stderr, "Error : connect()\n");
      return -1;
  }

  return 0;
}
