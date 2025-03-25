CC = gcc
CFLAGS = -Wall -Wno-deprecated-declarations -I../shared
LDFLAGS = -lcrypto -lpthread

all: server

server: server.c ../shared/protocol.c ../shared/crypto.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm server
