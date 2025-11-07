# Makefile to build the CcspDhcpMgr binary and ensure local libraries are built

# Compiler
CC ?= gcc

# Compiler flags (adjust include paths as needed)
CFLAGS ?= -Wall -Wextra -O2 -I. -I./DHCP_RBUS_COM -I./SM_DHCPMGR -I./Target_Layer

# Linker flags and libraries
# LIBPATHS points to the local lib directories produced by the subcomponents
LIBPATHS := -L$(CURDIR)/DHCP_RBUS_COM -L$(CURDIR)/SM_DHCPMGR -L$(CURDIR)/Target_Layer
LDLIBS := -ldhcpmgr_rbus -lsm_dhcpmgr -ldhcp_server_v4 -lrbus -lcjson -lrt -lpthread

# Source and target
SRCS := CcspDHCPMgr.c
TARGET := CcspDhcpMgr
OBJS := $(SRCS:.c=.o)

# Sub-projects that produce libraries
SUBDIRS := DHCP_RBUS_COM SM_DHCPMGR Target_Layer

.PHONY: all libs clean distclean

all: libs $(TARGET)

# Ensure sub-project libraries are built first
libs:
	@for d in $(SUBDIRS); do \
		if [ -f $$d/Makefile ]; then \
			$(MAKE) -C $$d || exit 1; \
		else \
			echo "Warning: $$d/Makefile not found, skipping"; \
		fi; \
	done

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBPATHS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJS) $(TARGET)
	@for d in $(SUBDIRS); do \
		if [ -f $$d/Makefile ]; then \
			$(MAKE) -C $$d clean || true; \
		fi; \
	done

distclean: clean
	@for d in $(SUBDIRS); do \
		if [ -f $$d/Makefile ]; then \
			$(MAKE) -C $$d distclean || true; \
		fi; \
	done

# Notes:
# - Ensure rbus and cjson development libraries/headers are installed or adjust CFLAGS/LDFLAGS.
# - If libraries are installed in non-standard locations, set LD_LIBRARY_PATH or adjust LIBPATHS.