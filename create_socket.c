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

#include "create_socket.h"

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){
	int sfd = 0, err = 0;

	sfd = socket(AF_INET6, SOCK_DGRAM, 0);
	if(sfd == -1){
		fprintf(stderr, "Error: errno=%d %s\n", errno, strerror(errno));
		return -1;
	}

	struct sockaddr_in6 address;
	if(source_addr != NULL){
		memcpy(&address, source_addr, sizeof(address));
		address.sin6_port = htons(src_port);
		err = bind(sfd, (struct sockaddr*)&address, sizeof(address));
		if(err == -1){
			return -1;
		}
	} else if(dest_addr != NULL){
		memcpy(&address, dest_addr, sizeof(address));
		address.sin6_port = htons(dst_port);
		err = connect(sfd, (struct sockaddr*)&address, sizeof(address));
		if(err == -1){
			return -1;
		}
	}
	else{
		fprintf(stderr, "source_addr == NULL && dest_addr == NULL\n");
		return -1;
	}

	return sfd;
}
