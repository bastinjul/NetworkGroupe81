#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "real_address.h"
#include "create_socket.h"
#include "wait_for_client.h"
#include "packet_interface.h"
#include "util.h"

//VARIABLES
const char* host;
int port, sfd;
struct sockaddr_in6 addr;
socklen_t addrlen;
FILE* outputfile;
int all_pkt_received = 1;
int lastack = -1;
int begin_window = 0;
int window_size = MAX_WINDOW_SIZE;
int nbr_arg;

//STRUCTURES
typedef enum pkt_received_state{
  ACK,
  FREE,
} pkt_received_state_t;

typedef struct pkt_received{
  pkt_t* pkt;
  pkt_received_state_t state;
} pkt_received_t;

struct __attribute__((__packed__)) pkt {
	struct {
		uint8_t window : 5;
		uint8_t tr : 1;
		uint8_t type : 2;
		uint8_t seqnum;
		uint16_t length;
		uint32_t timestamp;
		uint32_t crc1;
	} header;
	char* payload;
	uint32_t crc2;
};

//BUFFER
pkt_received_t BUFFER[BUFFER_SIZE];

//FUNCTIONS
int write_data(pkt_t* pkt, FILE* file);
void treat_data(char* data, int size);
void send_ack(uint16_t lenth, uint32_t timestamp, ptypes_t type);
int is_in_window(int seqnum);
void slide();
void add_to_buffer(pkt_t* pkt, int seqnum);

int main(int argc, const char* argv[]){
  nbr_arg = argc;

  if(!(argc == 3 || argc == 5)){
    perror("Invalid arguments\n Usage : ./receiver [-f/--file outfile] hostname port");
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

  const char* err = real_address(host, &addr);
  if(err != NULL){
    fclose(outputfile);
    perror(err);
    exit(EXIT_FAILURE);
  }
  addrlen = sizeof(addr);

  sfd = create_socket(&addr, port, NULL, -1);

  if(sfd < 0){
    fclose(outputfile);
    perror("Create socket");
    exit(EXIT_FAILURE);
  }

  char first_pkt[HEADER_SIZE + CRC1_SIZE + MAX_PAYLOAD_SIZE + CRC2_SIZE];
  int bytes_received = wait_for_client(sfd, first_pkt);
  if(bytes_received < 0){
    fclose(outputfile);
    perror("Wait for client");
    exit(EXIT_FAILURE);
  }
  treat_data(first_pkt, bytes_received);

  while(all_pkt_received){
    char  rec_pkt[HEADER_SIZE + CRC1_SIZE + MAX_PAYLOAD_SIZE + CRC2_SIZE];
    int b_received = recvfrom(sfd, rec_pkt, sizeof(rec_pkt), MSG_PEEK, (struct sockaddr *)&addr, &addrlen);
    if(b_received < 0){
      perror("recvfrom");
    }
    treat_data(rec_pkt, b_received);
  }

  fprintf(stdout, "End of transfer\n");
  fclose(outputfile);
  return EXIT_SUCCESS;
}

/*
 * Write pkt->payload in file
 */
int write_data(struct pkt* receive_pkt, FILE* file){
  if(nbr_arg == 5){
    if(fwrite(receive_pkt->payload, pkt_get_length(receive_pkt), 1, file) != pkt_get_length(receive_pkt)){
      perror("fwrite");
      return 0;
    }
    fflush(file);
  }
  else{
    fprintf(stdout, "Data Received : %s\n", receive_pkt->payload);
  }
  return 1;
}

/*
 *
 */
void treat_data(char* data, int size){
  pkt_t *received_pkt = pkt_new();
  pkt_status_code received_pkt_status = pkt_decode(data, size, received_pkt);

  //if packet if ok
  if(received_pkt_status == PKT_OK && pkt_get_type(received_pkt) == PTYPE_DATA){

    //if tr == 1
    if(!pkt_get_tr(received_pkt)){
      fprintf(stdout, "packet with tr == 1\n");
      send_ack(pkt_get_length(received_pkt), pkt_get_timestamp(received_pkt), PTYPE_NACK);
    }
    //if tr == 0
    else{
      int seqnum = pkt_get_seqnum(received_pkt)%MAX_SEQ_NUMBER;

      //if seqnum of received_packet is in window
      if(is_in_window(seqnum)){
        add_to_buffer(received_pkt, seqnum);
        window_size--;

        //if the packet is the first of the window we slide the window
        if(seqnum == begin_window){
          slide();
        }
        else{
          send_ack(pkt_get_length(received_pkt), pkt_get_timestamp(received_pkt), PTYPE_ACK);
        }
      }
      else{
        pkt_del(received_pkt);
      }
    }
  }
  else{
    pkt_del(received_pkt);
  }
}

/*
 *
 */
void send_ack(uint16_t length, uint32_t timestamp, ptypes_t type){
  if(length == 0){
    all_pkt_received = 0;
  }
  pkt_t* pkt_ack = pkt_new();
  pkt_set_type(pkt_ack, type);
  pkt_set_tr(pkt_ack, 0);
  pkt_set_window(pkt_ack, window_size);
  pkt_set_length(pkt_ack, 0);
  pkt_set_seqnum(pkt_ack, lastack);
  struct timeval *timev;
  timev = (struct timeval*)malloc(sizeof(struct timeval));
  if(gettimeofday(timev, NULL) != 0){
    perror("gettimeofday");
  }
  pkt_set_timestamp(pkt_ack, timeval_to_millisec(timev) - timestamp);
  free(timev);

  char data[HEADER_SIZE + CRC1_SIZE + MAX_PAYLOAD_SIZE + CRC2_SIZE];
  size_t size = sizeof(data);
  pkt_encode(pkt_ack, data, &size);
  int bytes_sent = sendto(sfd, data, HEADER_SIZE + CRC1_SIZE, 0, (struct sockaddr *) &addr, addrlen);
  if(bytes_sent < 0){
    perror("sendto");
  }
  fprintf(stdout, "Bytes send : %d\n", bytes_sent);

  pkt_del(pkt_ack);
}

/*
 * check if seqnum is in the window
 */
int is_in_window(int seqnum){
  int end_window = (begin_window + MAX_WINDOW_SIZE)%(BUFFER_SIZE-1);
  if(end_window < begin_window){
    if(seqnum <= end_window){
      return 1;
    }
    return 0;
  }
  else{
    if(seqnum <= end_window && seqnum >= begin_window){
      return 1;
    }
  }
  return 0;
}
/*
 * Slide the window and write the data
 */
void slide(){
  uint16_t length = 0;
  uint32_t timestamp = 0;
  while(BUFFER[begin_window].state == ACK){
    length = pkt_get_length(BUFFER[begin_window].pkt);
    timestamp = pkt_get_timestamp(BUFFER[begin_window].pkt);
    if(!write_data(BUFFER[begin_window].pkt, outputfile)){
      perror("write fail");
    }
    BUFFER[begin_window].state = FREE;
    pkt_del(BUFFER[begin_window].pkt);
    BUFFER[begin_window].pkt = NULL;
    begin_window = (begin_window+1)%(BUFFER_SIZE-1);
    window_size++;
    lastack = (lastack+1)%MAX_SEQ_NUMBER;
  }
  send_ack(length, timestamp, PTYPE_ACK);
}

/*
 *
 */
void add_to_buffer(pkt_t* pkt, int seqnum){
  BUFFER[seqnum].pkt = pkt;
  BUFFER[seqnum].state = ACK;
}
