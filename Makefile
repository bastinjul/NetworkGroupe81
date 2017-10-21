CC = gcc
CFLAGS = -Wall -Werror -Wshadow -Wextra -lz
DEPS =  src/real_address.h src/packet_interface.h src/create_socket.h src/create_socket.h src/wait_for_client.h src/read_write_loop.h src/jacobson.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
all: receiver sender

receiver: src/packet_implem.c src/receiver.o src/real_address.o src/create_socket.c src/wait_for_client.o
	$(CC) -o $@ $^ $(CFLAGS)
sender: src/packet_implem.c src/sender.o src/real_address.o src/create_socket.c src/jacobson.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	@rm -f receiver sender src/real_address.o src/receiver.o src/sender.o src/wait_for_client.o
