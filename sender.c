#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "real_address.h"
#include "create_socket.h"


const char* host;
int port, sfd;
FILE* inputfile;

int main(int argc, const char* argv[]){

  if(!(argc == 3 || argc == 5)){
    perror("Invalid arguments\n Usage : ./sender [-f/--filename] hostname port");
    exit(EXIT_FAILURE);
  }

  //-f
  if(argc == 5){
    port = atoi(argv[4]);
    host = argv[3];

    inputfile = fopen(argv[2], O_RDONLY);
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
    perror(err);
    exit(EXIT_FAILURE);
  }

  sfd = create_socket(NULL, -1, &addr, port);
  if(sfd < 0){
    perror("Create socket");
    close(sfd);
  }
}
