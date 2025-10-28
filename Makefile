# Makefile for CcspDHCPMgr

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -I/usr/local/include -I/usr/local/include/cjson -I/usr/include/cjson

# Linker flags
LDFLAGS = -L/usr/local/lib -lcjson -lrbus

# Source files
SRCS = dhcp_server_v4_apis.c sm_DhcpMgr_apis.c sm_DhcpMgr.c dhcpmgr_rbus_apis.c

# Header files
HDRS = dhcp_server_v4_apis.h sm_DhcpMgr.h dhcpmgr_rbus_apis.h

# Output binary
TARGET = CcspDHCPMgr

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link object files to create the binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files into object files
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean