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

#include "real_address.h"

/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char * real_address(const char* address, struct sockaddr_in6* rval){
	int error;
	const char* error_msg = NULL;

	struct addrinfo hints, *res, *res2;
	memset(&hints, 0, sizeof(hints));
    memset(rval, 0, sizeof(struct sockaddr_in6));

	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;

	if((error = getaddrinfo(address, NULL, &hints, &res)) != 0){
			error_msg = gai_strerror(error);
	}

	res2 = res;
	while(res){
		if(res->ai_family == AF_INET6){
			memcpy(rval, (struct sockaddr_in6*) res->ai_addr, sizeof(*rval));
			rval->sin6_family = AF_INET6;
			break;
		}
		res = res->ai_next;
	}

	freeaddrinfo(res2);

	return error_msg;
}
