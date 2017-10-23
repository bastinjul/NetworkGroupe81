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

#define STDOUT 1
#define STDIN 0

//VARIABLES
const char* host;
int port, sfd, filefd;
struct sockaddr_in6 addr;
socklen_t addrlen;
FILE* inputfile;
int nbr_arg;
int all_pkt_send = 0;
int all_pkt_read = 0;
int end_of_transmition = 0;
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
int all_pkt_acked();
int is_stop(char* buf);
int is_in_window(int seqnum);

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



  while(!all_pkt_send || !end_of_transmition){
    // fprintf(stdout, "===========================================================================================\n");
    // fprintf(stdout, "===========================================================================================\n");
    // fprintf(stdout, "seqnum = %d\n", seqnum);
    // fprintf(stdout, "begin_window = %d\n", begin_window);
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    FD_SET(STDIN, &readfds);
    FD_SET(filefd, &readfds);
    FD_SET(sfd, &readfds);
    FD_SET(sfd, &writefds);
    FD_SET(STDOUT, &writefds);
    if(select(nfds, &readfds, &writefds, NULL, NULL)==-1){
      perror("select");
      fclose(inputfile);
      exit(EXIT_FAILURE);
    }


    if (((FD_ISSET(filefd, &readfds)&&(nbr_arg == 5))||(FD_ISSET(STDIN, &readfds)&&(nbr_arg == 3))) && FD_ISSET(sfd, &writefds) && !all_pkt_send){
      fprintf(stdout, "Read file and send to receiver\n");
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
      fprintf(stdout, "Waiting ack\n");
      char data[HEADER_SIZE + CRC1_SIZE + MAX_PAYLOAD_SIZE + CRC2_SIZE];
      ssize_t read_bytes = read(sfd, data, sizeof(data));
      fprintf(stdout, "Ack receive, number of bytes received : %zd\n", read_bytes);
      pkt_t* ack_pkt = pkt_new();
      pkt_status_code ack_status_code = pkt_decode(data, sizeof(data), ack_pkt);
      fprintf(stdout, "Status of ack = %d\n", ack_status_code);
      if(ack_status_code == PKT_OK){
        fprintf(stdout, "decode data ok\n");
        if(pkt_get_type(ack_pkt) == PTYPE_ACK){
          fprintf(stdout, "ACK type\n");
          slide_window(pkt_get_seqnum(ack_pkt));
          size_receiver_window = pkt_get_window(ack_pkt);
        }
        else if(pkt_get_type(ack_pkt) == PTYPE_NACK){
          fprintf(stdout, "NACK TYPE\n");
          send_pkt(BUFFER[pkt_get_seqnum(ack_pkt)].pkt, pkt_get_seqnum(ack_pkt));
        }
        else{
          fprintf(stdout, "decode data != ok\n");
        }

      }

    }
    check_time();
    if(all_pkt_acked() && all_pkt_send){
      end_of_transmition = 1;
    }
  }

  send_last_pkt();

}

/*
 * Verify il all packet are acked
 */
int all_pkt_acked(){
  for(int i = 0; i<BUFFER_SIZE-1; i++){
    if(BUFFER[i].state == SEND){
      return 0;
    }
  }
  return 1;
}

/*
 * Read data from file or stdin and put pkt in buffer
 * @return 1 on success
 */
int read_from_input(){
  //from stdin
  if(nbr_arg == 3){
    fprintf(stdout, "Read data from stdin\n");
    char buf[MAX_PAYLOAD_SIZE];
    int n = read(STDIN, buf, MAX_PAYLOAD_SIZE);
    fprintf(stdout, "Read data = %s\n", buf);
    if(n == -1){
      perror("read");
      return 0;
    }
    else{
      if(is_stop(buf)){
        fprintf(stdout, "C'EST STOP\n");
        all_pkt_read = 1;
      }

      pkt_t* pkt = pkt_new();
      pkt_set_type(pkt, PTYPE_DATA);
      pkt_set_tr(pkt, 0);
      pkt_set_window(pkt, window_size);
      pkt_set_seqnum(pkt, seqnum);
      pkt_set_length(pkt, n);
      pkt_set_payload(pkt, buf, n);

      fprintf(stdout, "payload = %s\n", pkt_get_payload(pkt));

      seqnum = (seqnum+1)%MAX_SEQ_NUMBER;
      fprintf(stdout, "new seqnum = %d \n", seqnum);
      add_to_buffer(pkt);
      return 1;
    }
  }
  //from file
  else{
    fprintf(stdout, "Read data from file\n");
    char buf[MAX_PAYLOAD_SIZE];
    int n = read(filefd, buf, MAX_PAYLOAD_SIZE);
    if(n == -1){
      perror("read");
      return 0;
    }
    else{
      fprintf(stdout, "Number of bytes read in file : %d\n", n);
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
      fprintf(stdout, "new seqnum = %d \n", seqnum);
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
  fprintf(stdout, "Sending last packet\n");
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
  fprintf(stdout, "Sending a packet with seqnum = %d\n", pkt_get_seqnum(pkt));
  fprintf(stdout, "And payload = %s\n", pkt_get_payload(pkt));
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

  fprintf(stdout, "Sending data\n");
  int bytes_sent = write(sfd, data, size);
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
  int end_window = (begin_window+MAX_WINDOW_SIZE)%(BUFFER_SIZE-1);
  fprintf(stdout, "ack_seqnum = %d, begin_window = %d, end_window = %d\n", ack_seqnum, begin_window, end_window);
  if(is_in_window(ack_seqnum)){
      fprintf(stdout, "ack_seqnum is in window\n");
    while(begin_window != (ack_seqnum+1)%(BUFFER_SIZE-1)){
      BUFFER[begin_window].state = ACKED;
      pkt_del(BUFFER[begin_window].pkt);
      BUFFER[begin_window].timeval = 0;
      begin_window = (begin_window+1)%(BUFFER_SIZE-1);
      fprintf(stdout, "begin_window = %d\n", begin_window);
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
      fprintf(stdout, "Resending data of packet in BUFFER[%d]\n", i);
      fprintf(stdout, "Difference of time = %d\n", (int)(timeval - BUFFER[i].timeval));
      send_pkt(BUFFER[i].pkt, i);
    }
  }
}

/*
 * Add packet to the buffer
 */
void add_to_buffer(pkt_t* pkt){
  fprintf(stdout, "Adding pkt to the buffer at position : %d\n", ptr_add_buffer);
  BUFFER[ptr_add_buffer].pkt = pkt;
  BUFFER[ptr_add_buffer].state = WAIT;
  ptr_add_buffer = (ptr_add_buffer+1)%(BUFFER_SIZE-1);
  fprintf(stdout, "ptr_add_buffer after = %d\n", ptr_add_buffer);
  window_size--;
}

int is_stop(char *buf){
  char* stop = "STOP";
  for(int i = 0; i<4; i++){
    if(buf[i] != stop[i]){
      return 0;
    }
  }
  return 1;
}


/*
 * check if seqnum is in the window
 */
int is_in_window(int seq){
  int end_window = (begin_window + MAX_WINDOW_SIZE)%(BUFFER_SIZE-1);
  if(end_window < begin_window){
    if((seq <= end_window && seq>=0) || (seq >= begin_window && seq < MAX_WINDOW_SIZE)){
      return 1;
    }
    return 0;
  }
  else{
    if(seq <= end_window && seq >= begin_window){
      return 1;
    }
  }
  return 0;
}
