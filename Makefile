# TODO: make sure the rules for server client and markdown filled!
CC := gcc
CFLAGS := -Wall -std=c99 -D_POSIX_C_SOURCE=200809L -lpthread -fsanitize=address

all: server client

all: server client

server: src/server.c src/document.c src/helper.c src/markdown.c src/user.c
	$(CC) $(CFLAGS) -g -o server src/server.c src/document.c src/helper.c src/markdown.c src/user.c

client: src/client.c src/document.c src/helper.c src/markdown.c src/user.c
	$(CC) $(CFLAGS) -g -o client src/client.c src/document.c src/helper.c src/markdown.c src/user.c

markdown.o: src/markdown.c src/document.c src/helper.c src/user.c
	$(CC) $(CFLAGS) -c src/document.c src/helper.c src/user.c src/markdown.c
	ld -r document.o helper.o user.o markdown.o -o temp.o
	mv temp.o markdown.o
	rm -f document.o helper.o user.o

clean:
	find . -type p -name "FIFO_*" -exec unlink {} \;
	rm -f server client markdown.o
