CC = gcc
CFLAGS = -Wall -Werror -Wshadow -Wextra -lz -std=gnu99
DEPS =  src/real_address.h src/packet_interface.h  src/create_socket.h src/wait_for_client.h src/jacobson.h src/util.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
all: receiver sender

receiver: src/packet_implem.o src/receiver.o src/real_address.o src/create_socket.o src/wait_for_client.o src/util.o
	$(CC) -o $@ $^ $(CFLAGS)
sender: src/packet_implem.o src/sender.o src/real_address.o src/create_socket.o src/jacobson.o src/util.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	@rm -f receiver sender src/real_address.o src/receiver.o src/sender.o src/wait_for_client.o src/jacobson.o src/packet_implem.o src/util.o src/create_socket.o
