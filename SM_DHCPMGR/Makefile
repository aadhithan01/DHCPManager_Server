
CC ?= gcc
AR ?= ar
ARFLAGS ?= rcs
RANLIB ?= ranlib

PREFIX ?= /usr/local
DESTDIR ?=

# Additional include directories (one can override when calling make)
INC_DIRS ?= -I../Target_Layer -I. -I../DHCP_RBUS_COM

CFLAGS ?= -O2 -fPIC -Wall -Wextra
# Disable warn_unused_result warnings (e.g. from system/fgets marked warn_unused_result)
CFLAGS += -Wno-unused-result
LDFLAGS ?=
# By default link against the Target_Layer library so symbols used from
# dhcp_server_v4_apis.h are resolved at link time.
# You can override or extend this on the make command line.
LDFLAGS += -L../Target_Layer -ldhcp_server_v4

# If you prefer to embed the static archive instead of linking the shared
# library, call make with STATIC_EMBED=1 and the static lib will be
# included during the shared link step.
STATIC_EMBED ?= 0

SRCS := sm_DhcpMgr.c sm_DhcpMgr_apis.c
OBJS := $(SRCS:.c=.o)

LIBNAME := sm_dhcpmgr
TARGET_STATIC := lib$(LIBNAME).a
TARGET_SHARED := lib$(LIBNAME).so
TARGET_LA := $(TARGET_STATIC:.a=.la)

.PHONY: all clean install uninstall

all: $(TARGET_STATIC) $(TARGET_SHARED) $(TARGET_LA)

$(TARGET_STATIC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@


$(TARGET_SHARED): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(TARGET_LA): $(TARGET_SHARED) $(TARGET_STATIC)
	@echo "dlname='$(TARGET_SHARED)'" > $@
	@echo "library_names='$(TARGET_SHARED) $(TARGET_STATIC)'" >> $@
	@echo "old_library='$(TARGET_STATIC)'" >> $@
	@echo "inherited_linker_flags=''" >> $@
	@echo "dependency_libs=''" >> $@
	@echo "installed=no" >> $@
	@echo "shouldnotlink=no" >> $@
	@echo "libdir='$(PREFIX)/lib'" >> $@

%.o: %.c sm_DhcpMgr.h
	$(CC) $(CFLAGS) $(INC_DIRS) -c $< -o $@


install: all
	install -d "$(DESTDIR)$(PREFIX)/lib"
	install -m 644 $(TARGET_STATIC) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 755 $(TARGET_SHARED) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 644 $(TARGET_LA) "$(DESTDIR)$(PREFIX)/lib/"
	install -d "$(DESTDIR)$(PREFIX)/include"
	install -m 644 sm_DhcpMgr.h "$(DESTDIR)$(PREFIX)/include/"


uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/lib/$(TARGET_STATIC)" "$(DESTDIR)$(PREFIX)/lib/$(TARGET_SHARED)" "$(DESTDIR)$(PREFIX)/lib/$(TARGET_LA)" "$(DESTDIR)$(PREFIX)/include/sm_DhcpMgr.h"

clean:
	rm -f $(OBJS) $(TARGET_STATIC) $(TARGET_SHARED)

# Usage:
#  make                -> builds both libsm_dhcpmgr.a and libsm_dhcpmgr.so
#  make clean
#  make install PREFIX=/some/prefix  (or set DESTDIR=/staging)
