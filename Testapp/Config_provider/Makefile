# Makefile for rbus test client (Rbus_client_test)

# Compiler
CC = gcc

CFLAGS = -Wall -Wextra -I/usr/local/include -I/usr/local/include/cjson -I/usr/include/cjson

# Linker flags
LDFLAGS = -L/usr/local/lib -lcjson -lrbus

# Source files
SRCS = Rbus_client_test1.c

# Output binary
TARGET = ../config_provider_Rbus_client_test

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link object files to create the binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

# Usage examples:
#  make                # builds using defaults
#  make RBUS_DIR=/path/to/rbus    # override rbus location
#  make CFLAGS="$(CFLAGS) -I/some/other/include" LDFLAGS="-L/other/lib -lname"
