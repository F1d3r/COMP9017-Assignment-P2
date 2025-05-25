# TODO: make sure the rules for server client and markdown filled!
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L -lpthread

all: server client

server: src/server.c src/document.c src/helper.c src/markdown.c src/user.c
	$(CC) $(CFLAGS) -o server src/server.c src/document.c src/helper.c src/markdown.c src/user.c

client: src/client.c src/document.c src/helper.c src/markdown.c src/user.c
	$(CC) $(CFLAGS) -o client src/client.c src/document.c src/helper.c src/markdown.c src/user.c

markdown.o: src/markdown.c
	$(CC) $(CFLAGS) -c src/markdown.c -o markdown.o

clean:
	find . -type p -name "FIFO_*" -exec unlink {} \;
	rm -f server client markdown.o
