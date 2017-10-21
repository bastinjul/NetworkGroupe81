#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "real_address.h"
#include "create_socket.h"
#include "jacobson.h"


const char* host;
int port, sfd;
FILE* inputfile;

int main(int argc, const char* argv[]){

  if(!(argc == 3 || argc == 5)){
    perror("Invalid arguments\n Usage : ./sender [-f/--file infile] hostname port");
    exit(EXIT_FAILURE);
  }

  //-f
  if(argc == 5){
    port = atoi(argv[4]);
    host = argv[3];

    inputfile = fopen(argv[2], "r");
    if(inputfile == NULL){
      perror("Opening File");
      exit(EXIT_FAILURE);
    }
  }
  else{
    port = atoi(argv[2]);
    host = argv[1];
  }

  struct sockaddr_in6 addr;

  const char* err = real_address(host, &addr);
  if(err != NULL){
    fclose(inputfile);
    perror(err);
    exit(EXIT_FAILURE);
  }

  sfd = create_socket(NULL, -1, &addr, port);
  if(sfd < 0){
    perror("Create socket");
    fclose(inputfile);
  }

  fd_set readfds, writefds;
	int nfds = sfd + 1;

	fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL) | O_NONBLOCK);
}
