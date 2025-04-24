CC = gcc
CFLAGS = -Wall -Wno-deprecated-declarations -D_XOPEN_SOURCE_EXTENDED -I../shared
LDFLAGS = -lcrypto -lncursesw -lpthread

all: client

client: client.c ui.c ../shared/protocol.c ../shared/crypto.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm client