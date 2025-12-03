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

SRCS_V4 := dhcp_server_v4_apis.c
SRCS_V6 := dhcp_server_v6_apis.c

OBJS_V4 := $(SRCS_V4:.c=.o)
OBJS_V6 := $(SRCS_V6:.c=.o)
OBJS := $(OBJS_V4) $(OBJS_V6)

LIBNAME_V4 := dhcp_server_v4
TARGET_STATIC_V4 := lib$(LIBNAME_V4).a
TARGET_SHARED_V4 := lib$(LIBNAME_V4).so
TARGET_LA_V4 := $(TARGET_STATIC_V4:.a=.la)

LIBNAME_V6 := dhcp_server_v6
TARGET_STATIC_V6 := lib$(LIBNAME_V6).a
TARGET_SHARED_V6 := lib$(LIBNAME_V6).so
TARGET_LA_V6 := $(TARGET_STATIC_V6:.a=.la)

.PHONY: all clean install uninstall

all: $(TARGET_STATIC_V4) $(TARGET_SHARED_V4) $(TARGET_LA_V4) \
     $(TARGET_STATIC_V6) $(TARGET_SHARED_V6) $(TARGET_LA_V6)

$(TARGET_STATIC_V4): $(OBJS_V4)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

$(TARGET_STATIC_V6): $(OBJS_V6)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

$(TARGET_SHARED_V4): $(OBJS_V4)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(TARGET_SHARED_V6): $(OBJS_V6)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(TARGET_LA_V4): $(TARGET_SHARED_V4) $(TARGET_STATIC_V4)
	@echo "dlname='$(TARGET_SHARED_V4)'" > $@
	@echo "library_names='$(TARGET_SHARED_V4) $(TARGET_STATIC_V4)'" >> $@
	@echo "old_library='$(TARGET_STATIC_V4)'" >> $@
	@echo "inherited_linker_flags=''" >> $@
	@echo "dependency_libs=''" >> $@
	@echo "installed=no" >> $@
	@echo "shouldnotlink=no" >> $@
	@echo "libdir='$(PREFIX)/lib'" >> $@

$(TARGET_LA_V6): $(TARGET_SHARED_V6) $(TARGET_STATIC_V6)
	@echo "dlname='$(TARGET_SHARED_V6)'" > $@
	@echo "library_names='$(TARGET_SHARED_V6) $(TARGET_STATIC_V6)'" >> $@
	@echo "old_library='$(TARGET_STATIC_V6)'" >> $@
	@echo "inherited_linker_flags=''" >> $@
	@echo "dependency_libs=''" >> $@
	@echo "installed=no" >> $@
	@echo "shouldnotlink=no" >> $@
	@echo "libdir='$(PREFIX)/lib'" >> $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -d "$(DESTDIR)$(PREFIX)/lib"
	install -m 644 $(TARGET_STATIC_V4) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 755 $(TARGET_SHARED_V4) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 644 $(TARGET_LA_V4) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 644 $(TARGET_STATIC_V6) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 755 $(TARGET_SHARED_V6) "$(DESTDIR)$(PREFIX)/lib/"
	install -m 644 $(TARGET_LA_V6) "$(DESTDIR)$(PREFIX)/lib/"
	install -d "$(DESTDIR)$(PREFIX)/include"
	install -m 644 dhcp_server_v4_apis.h "$(DESTDIR)$(PREFIX)/include/"
	install -m 644 dhcp_server_v6_apis.h "$(DESTDIR)$(PREFIX)/include/"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/lib/$(TARGET_STATIC_V4)" "$(DESTDIR)$(PREFIX)/lib/$(TARGET_SHARED_V4)" "$(DESTDIR)$(PREFIX)/lib/$(TARGET_LA_V4)"
	rm -f "$(DESTDIR)$(PREFIX)/lib/$(TARGET_STATIC_V6)" "$(DESTDIR)$(PREFIX)/lib/$(TARGET_SHARED_V6)" "$(DESTDIR)$(PREFIX)/lib/$(TARGET_LA_V6)"
	rm -f "$(DESTDIR)$(PREFIX)/include/dhcp_server_v4_apis.h" "$(DESTDIR)$(PREFIX)/include/dhcp_server_v6_apis.h"

clean:
	rm -f $(OBJS_V4) $(TARGET_STATIC_V4) $(TARGET_SHARED_V4) $(TARGET_LA_V4) \
		  $(OBJS_V6) $(TARGET_STATIC_V6) $(TARGET_SHARED_V6) $(TARGET_LA_V6)

# Usage:
#  make                -> builds both libdhcp_server_v4.a/.so and libdhcp_server_v6.a/.so
#  make clean
#  make install PREFIX=/some/prefix  (or set DESTDIR=/staging)
