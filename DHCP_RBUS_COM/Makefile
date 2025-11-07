# Makefile for DHCP_RBUS_COM
# Builds a static and shared library for the RBUS DHCP Manager component.
#
# Usage:
#  make               - builds libdhcpmgr_rbus.a and libdhcpmgr_rbus.so in the current dir
#  make DESTDIR=../lib - build and create destination dir
#  make install       - copies libraries and public header to DESTDIR (default ../lib)
#
# Configuration:
#  CC, CFLAGS, LDFLAGS can be overridden on the make command line.
#  RBUS_CFLAGS and RBUS_LDFLAGS are provided as convenience variables for rbus-specific flags.

CC ?= gcc
CFLAGS ?= -fPIC -Wall -Wextra -O2 -I. $(RBUS_CFLAGS) -I/usr/local/include 
LDFLAGS ?= $(RBUS_LDFLAGS) -L/usr/local/lib -lrbus

SRC := dhcpmgr_rbus_apis.c
OBJ := $(SRC:.c=.o)
HDR := dhcpmgr_rbus_apis.h

LIBNAME := dhcpmgr_rbus
STATIC_LIB := lib$(LIBNAME).a
SHARED_LIB := lib$(LIBNAME).so

DESTDIR ?= ../lib

all: $(STATIC_LIB) $(SHARED_LIB)

$(STATIC_LIB): $(OBJ)
	@echo "Creating static library $@"
	ar rcs $@ $^

$(SHARED_LIB): $(OBJ)
	@echo "Creating shared library $@"
	$(CC) -shared -o $@ $^ $(LDFLAGS)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

install: all | $(DESTDIR)
	@echo "Installing libraries and header to $(DESTDIR)"
	cp -v $(STATIC_LIB) $(DESTDIR)/
	cp -v $(SHARED_LIB) $(DESTDIR)/
	cp -v $(HDR) $(DESTDIR)/

$(DESTDIR):
	mkdir -p $(DESTDIR)

clean:
	@echo "Cleaning build artifacts"
	rm -f $(OBJ) $(STATIC_LIB) $(SHARED_LIB)

.PHONY: all clean install
