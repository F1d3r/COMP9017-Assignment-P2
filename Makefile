# Compiler settings
CC := gcc
CFLAGS := -Wall -fsanitize=address

# Executables
TARGETS := server client

# Default target
all: $(TARGETS)

server: server.o markdown.o document.o helper.o
	$(CC) $(CFLAGS) -o bin/server bin/server.o bin/markdown.o bin/document.o bin/helper.o
	
client: client.o markdown.o document.o helper.o
	$(CC) $(CFLAGS) -o bin/client bin/client.o bin/markdown.o bin/document.o bin/helper.o

server.o: src/server.c
	$(CC) $(CFLAGS) -c src/server.c -o bin/server.o

client.o: src/client.c
	$(CC) $(CFLAGS) -c src/client.c -o bin/client.o

markdown.o: src/markdown.c
	$(CC) $(CFLAGS) -c src/markdown.c -o bin/markdown.o

document.o: src/document.c
	$(CC) $(CFLAGS) -c src/document.c -o bin/document.o

helper.o: src/helper.c
	$(CC) $(CFLAGS) -c src/helper.c -o bin/helper.o

# Clean build artifacts
clean:
	find . -type p -name "FIFO_*" -exec unlink {} \;
	rm -f bin/*