#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "real_address.h"
#include "create_socket.h"
#include "wait_for_client.h"
#include "packet_interface.h"

const char* host;
int port, sfd;
FILE* outputfile;

int main(int argc, const char* argv[]){

  if(argc == 1){
    printf("%s\n", argv[0]);
  }
  //=======================Test======================
  pkt_t* pkt = pkt_new();

  pkt_set_type(pkt, PTYPE_DATA);
  pkt_set_tr(pkt, 0);
  pkt_set_window(pkt, 5);
  pkt_set_seqnum(pkt, 0);
  char* payload = "test";
  pkt_set_length(pkt, sizeof(payload));
  pkt_set_timestamp(pkt, 0);
  pkt_set_payload(pkt, payload, sizeof(payload));

  char* buf = calloc(1, 3000);
  size_t* size = (size_t *)malloc(sizeof(uint16_t));
  *size = 3000;
  pkt_status_code status = pkt_encode(pkt, buf, size);
  printf("status encode: %d\n", status);
  printf("size of buffer : %lu\n", sizeof(buf));
  pkt_t* newPkt = pkt_new();
  pkt_status_code decodeStatus = pkt_decode(buf, sizeof(buf), newPkt);
  printf("status decode : %d\n", decodeStatus);
  const char* decPayload = pkt_get_payload(newPkt);
  printf("payload decode : %s\n", decPayload);
  free(buf);
  pkt_del(pkt);

  /**
  if(!(argc == 3 || argc == 5)){
    perror("Invalid arguments\n Usage : ./sender [-f/--filename] hostname port");
    exit(EXIT_FAILURE);
  }

  //-f
  if(argc == 5){
    port = atoi(argv[4]);
    host = argv[3];

    outputfile = fopen(argv[2], "w");
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
*/

}
