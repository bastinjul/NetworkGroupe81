#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "real_address.h"
#include "create_socket.h"
#include "wait_for_client.h"

const char* host;
int port, sfd;
FILE* outputfile;

int main(int argc, const char* argv[]){

  if(!(argc == 3 || argc == 5)){
    perror("Invalid arguments\n Usage : ./sender [-f/--filename] hostname port");
    exit(EXIT_FAILURE);
  }

  //-f
  if(argc == 5){
    port = atoi(argv[4]);
    host = argv[3];

    outputfile = fopen(argv[2], O_WRONLY | O_CREATE | O_TRUNC);
    if(outputfile == NULL){
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

  sfd = create_socket(&addr, port, NULL, -1);

  if(sfd < 0){
    perror("Create socket");
    close(sfd);
  }

  if(wait_for_client(sfd) < 0){
    perror("Wait for client");
    close(sfd);
  }

  
}
