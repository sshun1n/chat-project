CC = gcc
CFLAGS = -Wall -Wno-deprecated-declarations -D_XOPEN_SOURCE_EXTENDED -I../shared
LDFLAGS = -lcrypto -lncursesw

all: client

client: client.c ../shared/protocol.c ../shared/crypto.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm client
