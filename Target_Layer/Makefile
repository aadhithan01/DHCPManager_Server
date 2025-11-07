CC ?= gcc
AR ?= ar
ARFLAGS ?= rcs
RANLIB ?= ranlib

PREFIX ?= /usr/local
DESTDIR ?=

CFLAGS ?= -O2 -fPIC -Wall -Wextra -I.
# Disable warn_unused_result warnings (e.g. from system/fgets marked warn_unused_result)
CFLAGS += -Wno-unused-result
LDFLAGS ?=

SRCS := dhcp_server_v4_apis.c
OBJS := $(SRCS:.c=.o)

LIBNAME := dhcp_server_v4
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

%.o: %.c dhcp_server_v4_apis.h
	$(CC) $(CFLAGS) -c $< -o $@


install: all
	install -d "$(DESTDIR)$(PREFIX)/lib"
	install -m 644 $(TARGET_STATIC) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 755 $(TARGET_SHARED) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 644 $(TARGET_LA) "$(DESTDIR)$(PREFIX)/lib/"
	install -d "$(DESTDIR)$(PREFIX)/include"
	install -m 644 dhcp_server_v4_apis.h "$(DESTDIR)$(PREFIX)/include/"


uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/lib/$(TARGET_STATIC)" "$(DESTDIR)$(PREFIX)/lib/$(TARGET_SHARED)" "$(DESTDIR)$(PREFIX)/lib/$(TARGET_LA)" "$(DESTDIR)$(PREFIX)/include/dhcp_server_v4_apis.h"

clean:
	rm -f $(OBJS) $(TARGET_STATIC) $(TARGET_SHARED)

# Usage:
#  make                -> builds both libdhcp_server_v4.a and libdhcp_server_v4.so
#  make clean
#  make install PREFIX=/some/prefix  (or set DESTDIR=/staging)
