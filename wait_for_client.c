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
/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd){
  char buf[1024];
  socklen_t socklen;
  struct sockaddr_storage addr;

  memset(&addr, 0, sizeof(addr));
  socklen = sizeof(addr);
  ssize_t bytes_received = recvfrom(sfd, buf, sizeof(buf), MSG_PEEK,
      (struct sockaddr *)&addr, &socklen);
  if (bytes_received == -1) {
      fprintf(stderr, "Impossible de lire un message venant du client\n");
      return -1;
  } else {
      int c = connect(sfd, (struct sockaddr *)&addr, socklen);
      if (c == -1) {
          fprintf(stderr, "Impossible de se connecter\n");
          return -1;
      }
  }

  return 0;
}
