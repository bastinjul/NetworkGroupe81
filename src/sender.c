#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#include "real_address.h"
#include "create_socket.h"
#include "jacobson.h"
#include "util.h"
#include "packet_interface.h"

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
int ptr_sending = 0;
const uint32_t max_timeval = 2000;
uint8_t window_size = MAX_WINDOW_SIZE;


//STRUCTURES
typedef enum{
  WAIT,
  ACKED,
  FREE,
  SEND,
} state_t;

typedef struct pkt_sender{
  pkt_t* pkt;
  uint32_t timeval;
  state_t state;
} pkt_sender_t;

//BUFFER
pkt_sender_t BUFFER[BUFFER_SIZE];

//FUNCTIONS
int read_from_input();
void send_last_pkt();
void send_pkt(pkt_t* pkt, int position_in_buffer);
void slide_window(uint8_t ack_seqnum);
void check_time();
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
    BUFFER[i].timeval = 0;
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
    if (((FD_ISSET(filefd, &readfds)&&(nbr_arg == 5))||(FD_ISSET(STDIN_FILENO, &readfds)&&(nbr_arg == 3))) && FD_ISSET(sfd, &writefds)){
      if(!read_from_input()){
        perror("read_from_input");
      }
      while (BUFFER[ptr_sending].state == WAIT && size_receiver_window > 0) {
        send_pkt(BUFFER[ptr_sending].pkt, ptr_sending);
        ptr_sending = (ptr_sending+1)%(BUFFER_SIZE-1);
      }
      all_pkt_send = all_pkt_read;

    }
    if(FD_ISSET(sfd, &readfds)){
      char data[HEADER_SIZE + CRC1_SIZE];
      ssize_t read_bytes = read(sfd, data, HEADER_SIZE + CRC1_SIZE);
      pkt_t* ack_pkt = pkt_new();
      if(pkt_decode(data, read_bytes, ack_pkt) == PKT_OK){
        if(pkt_get_type(ack_pkt) == PTYPE_ACK){
          slide_window(pkt_get_seqnum(ack_pkt));
          size_receiver_window = pkt_get_window(ack_pkt);
        }
        else if(pkt_get_type(ack_pkt) == PTYPE_NACK){
          send_pkt(BUFFER[pkt_get_seqnum(ack_pkt)].pkt, pkt_get_seqnum(ack_pkt));
        }
      }

    }
    check_time();
  }
  send_last_pkt();
}

/*
 * Read data from file or stdin and put pkt in buffer
 * @return 1 on success
 */
int read_from_input(){
  //from stdin
  if(nbr_arg == 3){
    char buf[MAX_PAYLOAD_SIZE];
    int n = read(STDIN_FILENO, buf, MAX_PAYLOAD_SIZE);
    if(n == -1){
      perror("read");
      return 0;
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
      return 1;
    }
  }
  //from file
  else{
    char buf[MAX_PAYLOAD_SIZE];
    int n = read(filefd, buf, MAX_PAYLOAD_SIZE);
    if(n == -1){
      perror("read");
      return 0;
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
      return 1;
    }
  }
  return 0;
}

/*
 * Send the last packet with empty payload
 */
void send_last_pkt(){
  pkt_t* lastpkt = pkt_new();
  pkt_set_type(lastpkt, PTYPE_DATA);
  pkt_set_tr(lastpkt, 0);
  pkt_set_window(lastpkt, window_size);
  pkt_set_seqnum(lastpkt, seqnum);
  pkt_set_length(lastpkt, 0);
  struct timeval timev;
  if(gettimeofday(&timev, NULL) != 0){
    perror("gettimeofday");
  }
  pkt_set_timestamp(lastpkt, timeval_to_millisec(&timev));

  char data[HEADER_SIZE + CRC1_SIZE + MAX_PAYLOAD_SIZE + CRC2_SIZE];
  size_t size = sizeof(data);
  pkt_encode(lastpkt, data, &size);

  int bytes_sent = write(sfd, data, sizeof(data));
  if(bytes_sent < 0){
    perror("write");
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
  uint32_t timestamp = timeval_to_millisec(&timev);
  pkt_set_timestamp(pkt, timestamp);

  BUFFER[position_in_buffer].timeval = timestamp;

  char data[HEADER_SIZE + CRC1_SIZE + MAX_PAYLOAD_SIZE + CRC2_SIZE];
  size_t size = sizeof(data);
  pkt_encode(pkt, data, &size);

  int bytes_sent = write(sfd, data, sizeof(data));
  if(bytes_sent < 0){
    perror("write2");
  }
  fprintf(stdout, "Bytes send2 : %d\n", bytes_sent);

  BUFFER[position_in_buffer].state = SEND;
}

/*
 * Slide the window of sending and free acked packets
 */
void slide_window(uint8_t ack_seqnum){
  if(((begin_window < (begin_window+MAX_WINDOW_SIZE)%(BUFFER_SIZE-1)) && (ack_seqnum >= begin_window))||
    ((begin_window > (begin_window+MAX_WINDOW_SIZE)%(BUFFER_SIZE-1))&&((ack_seqnum<begin_window+MAX_WINDOW_SIZE)||(ack_seqnum>begin_window)))){
    while(begin_window != (ack_seqnum+1)%(BUFFER_SIZE-1)){
      BUFFER[begin_window].state = ACKED;
      pkt_del(BUFFER[begin_window].pkt);
      BUFFER[begin_window].timeval = 0;
      begin_window = (begin_window+1)%(BUFFER_SIZE-1);
    }
  }

}

/*
 * Check the time of all packets of window
 * Resend all packet out of time and restart timer
 */
void check_time(){
  for(int i = begin_window; (begin_window < (begin_window+MAX_WINDOW_SIZE)%(BUFFER_SIZE-1) && i<begin_window+MAX_WINDOW_SIZE)
                          || (begin_window > (begin_window+MAX_WINDOW_SIZE)%(BUFFER_SIZE-1) && ((i<begin_window+MAX_WINDOW_SIZE)||(i>begin_window)));
                          i = (i+1)%(BUFFER_SIZE-1)){
    struct timeval timev;
    if(gettimeofday(&timev, NULL) != 0){
      perror("gettimeofday");
    }
    uint32_t timeval = timeval_to_millisec(&timev);
    if(timeval - BUFFER[i].timeval > max_timeval && BUFFER[i].state == SEND){
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
