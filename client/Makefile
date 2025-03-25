CC = gcc
CFLAGS = -Wall -I../shared
LDFLAGS = -lcrypto

all: client

client: client.c ../shared/protocol.c ../shared/crypto.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm client
