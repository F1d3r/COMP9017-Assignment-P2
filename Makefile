# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -fsanitize=address

# Directories
SRC_DIR := src
LIBS_DIR := libs
BIN_DIR := bin

# Ensure bin directory exists
$(shell mkdir -p $(BIN_DIR))

# Source files
SRCS := $(SRC_DIR)/server.c $(SRC_DIR)/client.c $(SRC_DIR)/document.c \
        $(SRC_DIR)/helper.c $(SRC_DIR)/markdown.c

# Header files
HDRS := $(LIBS_DIR)/document.h $(LIBS_DIR)/helper.h $(LIBS_DIR)/markdown.h

# Object files
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRCS))

# Executables
TARGETS := server client

# Default target
all: $(TARGETS)

# Compile individual object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Link server
server: $(BIN_DIR)/server.o $(BIN_DIR)/document.o $(BIN_DIR)/helper.o $(BIN_DIR)/markdown.o
	$(CC) $(CFLAGS) $^ -o $@

# Link client
client: $(BIN_DIR)/client.o $(BIN_DIR)/document.o $(BIN_DIR)/helper.o $(BIN_DIR)/markdown.o
	$(CC) $(CFLAGS) $^ -o $@

# Clean build artifacts
clean:
	rm -f $(BIN_DIR)/*.o $(TARGETS)