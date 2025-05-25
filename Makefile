# Compiler settings
CC := gcc
CFLAGS := -Wall -fsanitize=address -g
LINKS := -lpthread

# Executables
TARGETS := server client

# Default target
all: $(TARGETS)

server: server.o markdown.o document.o helper.o
	$(CC) $(CFLAGS) -o server server.o markdown.o document.o helper.o $(LINKS)
	
client: client.o markdown.o document.o helper.o
	$(CC) $(CFLAGS) -o client client.o markdown.o document.o helper.o $(LINKS)

server.o: src/server.c
	$(CC) $(CFLAGS) -c src/server.c -o server.o

client.o: src/client.c
	$(CC) $(CFLAGS) -c src/client.c -o client.o

markdown.o: src/markdown.c
	$(CC) $(CFLAGS) -c src/markdown.c -o markdown.o

document.o: src/document.c
	$(CC) $(CFLAGS) -c src/document.c -o document.o

helper.o: src/helper.c
	$(CC) $(CFLAGS) -c src/helper.c -o helper.o

# Clean build artifacts
clean:
	find . -type p -name "FIFO_*" -exec unlink {} \;
	rm -f *.o server client