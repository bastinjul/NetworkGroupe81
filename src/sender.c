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
#include "util.h"

//VARIABLES
const char* host;
int port, sfd, filefd;
struct sockaddr_in6 addr;
socklen_t addrlen;
FILE* inputfile;
int nbr_arg;
int all_pkt_send = 0;
int all_pkt_read = 0;
int size_receiver_window = 1;
int seqnum = 0;
int begin_window = 0;
int ptr_add_buffer = 0;
const uint32_t max_timeval = 2000;
uint8_t window_size = MAX_WINDOW_SIZE;

//BUFFER
pkt_sender_t BUFFER[BUFFER_SIZE];

//STRUCTURES
typedef enum{
  WAIT,
  ACKED,
  FREE,
  SENT,
} state_t;

typedef struct pkt_sender{
  pkt_t* pkt;
  uint32_t timeval;
  state_t state;
} pkt_sender_t;

//FUNCTIONS
int read_from_input(pkt_t* pkt);
void send_last_pkt();
void send_pkt(pkt_t* pkt, int position_in_buffer);
void slide_window(uint8_t ack_seqnum);
int check_time();
void add_to_buffer(pkt_t* pkt);

int main(int argc, const char* argv[]){
  nbr_arg = argc;

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
    fprintf(stdout, "Sending file %s\n", argv[2]);
    filefd = fileno(inputfile);
  }
  else{
    port = atoi(argv[2]);
    host = argv[1];
    fprintf(stdout, "Sending messages from stdin\n");
    fprintf(stdout, "Enter 'STOP' to stop the connection\n");
  }

  const char* err = real_address(host, &addr);
  if(err != NULL){
    fclose(inputfile);
    perror(err);
    exit(EXIT_FAILURE);
  }
  addrlen = sizeof(addr);

  sfd = create_socket(NULL, -1, &addr, port);
  if(sfd < 0){
    perror("Create socket");
    fclose(inputfile);
  }

  fd_set readfds, writefds;
	int nfds = sfd + 1;

	fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL) | O_NONBLOCK);

  for(int i = 0; i<BUFFER_SIZE; i++){
    BUFFER[i].state = FREE;
    BUFFER[i].pkt = NULL;
    BUFFER[i].timestamp = 0;
  }

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  FD_SET(STDIN_FILENO, &readfds);
  FD_SET(filefd, &readfds);
  FD_SET(sfd, &readfds);
  FD_SET(sfd, &writefds);
  FD_SET(STDOUT_FILENO, &writefds);
  if(select(nfds, &readfds, &writefds, NULL, NULL)==-1){
    perror("select");
    fclose(inputfile);
    exit(EXIT_FAILURE);
  }

  while(!all_pkt_send){


  }
  send_last_pkt();
}

/*
 * Read data from file or stdin and put pkt in buffer
 * @return 1 on success
 * @return 0 if eof or 'STOP' (stdin)
 */
int read_from_input(){
  //from stdin
  if(nbr_arg == 3){
    char buf[MAX_PAYLOAD_SIZE];
    int n = read(STDIN_FILENO, buf, MAX_PAYLOAD_SIZE);
    if(n == -1){
      perror("read");
    }
    else{
      if(strcmp(buf, "STOP") == 0){
        all_pkt_read = 1;
      }

      pkt_t* pkt = pkt_new();
      pkt_set_type(pkt, PTYPE_DATA);
      pkt_set_tr(pkt, 0);
      pkt_set_window(pkt, window_size);
      pkt_set_seqnum(pkt, seqnum);
      pkt_set_length(pkt, n);
      pkt_set_payload(pkt, buf, n);

      seqnum = (seqnum+1)%MAX_SEQ_NUMBER;
      add_to_buffer(pkt);
    }
  }
  //from file
  else{
    char buf[MAX_PAYLOAD_SIZE];
    int n = read(filefd, buf, MAX_PAYLOAD_SIZE);
    if(n == -1){
      perror("read");
    }
    else{
      if(n < MAX_PAYLOAD_SIZE){
        all_pkt_read = 1;
      }
      pkt_t* pkt = pkt_new();
      pkt_set_type(pkt, PTYPE_DATA);
      pkt_set_tr(pkt, 0);
      pkt_set_window(pkt, window_size);
      pkt_set_seqnum(pkt, seqnum);
      pkt_set_length(pkt, n);
      pkt_set_payload(pkt, buf, n);

      seqnum = (seqnum+1)%MAX_SEQ_NUMBER;
      add_to_buffer(pkt);
    }
  }
}

/*
 * Send the last packet with empty payload
 */
void send_last_pkt(){
  pkt_t* lastptk = pkt_new();
  pkt_set_type(lastpkt, PTYPE_DATA);
  pkt_set_tr(lastpkt, 0);
  pkt_set_window(lastpkt, window_size);
  pkt_set_seqnum(lastpkt, seqnum);
  pkt_set_length(lastpkt, 0);
  struct timeval *timev;
  timev = (struct timeval*)malloc(sizeof(struct timeval));
  if(gettimeofday(timev, NULL) != 0){
    perror("gettimeofday");
  }
  pkt_set_timestamp(lastpkt, timeval_to_millisec(timev));
  free(timev);

  char data[HEADER_SIZE + CRC1_SIZE + MAX_PAYLOAD_SIZE + CRC2_SIZE];
  size_t size = sizeof(data);
  pkt_encode(lastpkt, data, &size);

  int bytes_sent = sendto(sfd, data, HEADER_SIZE + CRC1_SIZE, 0, (struct sockaddr *) &addr, addrlen);
  if(bytes_sent < 0){
    perror("sendto");
  }
  fprintf(stdout, "Bytes send : %d\n", bytes_sent);
}

/*
 * Send a packet
 */
void send_pkt(pkt_t* pkt, int position_in_buffer){
  struct timeval timev;
  if(gettimeofday(&timev, NULL) != 0){
    perror("gettimeofday");
  }
  uint32_t timestamp = timeval_to_millisec(timev);
  pkt_set_timestamp(pkt, timestamp);
  free(timev);

  BUFFER[position_in_buffer].timestamp = timestamp;

  char data[HEADER_SIZE + CRC1_SIZE + MAX_PAYLOAD_SIZE + CRC2_SIZE];
  size_t size = sizeof(data);
  pkt_encode(pkt, data, &size);

  int bytes_sent = sendto(sfd, data, HEADER_SIZE + CRC1_SIZE, 0, (struct sockaddr *) &addr, addrlen);
  if(bytes_sent < 0){
    perror("sendto");
  }
  fprintf(stdout, "Bytes send : %d\n", bytes_sent);

  BUFFER[position_in_buffer].state = SEND;
}

/*
 * Slide the window of sending
 */
void slide_window(uint8_t ack_seqnum){

}

/*
 * Check the time of all packets of window
 * Resend all packet out of time and restart timer
 */
void check_time(){
  for(int i = begin_window; i>begin_window || i<begin_window+32; i = (i+1)%(BUFFER_SIZE-1)){
    struct timestamp timev;
    if(gettimeofday(&timev, NULL) != 0){
      perror("gettimeofday");
    }
    uint32_t timeval = timeval_to_millisec(timev);
    if(timeval - BUFFER[i].timestamp > max_timeval && BUFFER[i].state == SEND){
      send_pkt(BUFFER[i].pkt, i);
    }
  }
}

/*
 * Add packet to the buffer
 */
void add_to_buffer(pkt_t* pkt){
  BUFFER[ptr_add_buffer].pkt = pkt;
  BUFFER[ptr_add_buffer].state = WAIT;
  ptr_add_buffer = (ptr_add_buffer+1)%(BUFFER_SIZE-1);
  window_size--;
}
