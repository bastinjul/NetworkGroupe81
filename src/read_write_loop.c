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

#include "read_write_loop.h"

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(int sfd){
  int r;
  char buf[1024];

  struct pollfd fds[] = {
     { sfd, POLLIN | POLLOUT, 0},
     { 0, POLLIN, 0}
  };


  while(1) {
     memset(buf, '\0', sizeof(buf));
     r = poll(fds, 2, 1);

     if(r == -1)
         perror("Poll");
     if(r > 0) {
         if(fds[0].revents & POLLIN) {
             int nbr = read(sfd, buf, sizeof(buf));
             if(nbr == -1) {
                 perror("Read Socket");
             }else {
                 int nbw = write(STDOUT_FILENO, buf, nbr);
                 if(nbw == -1) {
                         perror("Write");
                         return;
                     }

                 if(nbw != nbr) {
                     perror("Number of bytes in socket");
                     return;
                 }
             }

         }
         else if(fds[1].revents & POLLIN) {
             int nbr = read(STDIN_FILENO, buf, sizeof(buf));
             if(nbr == -1) {
                 perror("Read");
             } else {
                     int nbw = write(sfd, buf, nbr);
                     if(nbw == -1) {
                         perror("Write socket");
                         return;
                     }

                 if(nbw != nbr) {
                         perror("Number of bytes in socket");
                         return;
                     }

             }
         }

     }
  }
}