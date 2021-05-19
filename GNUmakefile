PREFIX  ?= /usr/local
DESTDIR ?=

INSTALL ?= install
PKGCONF ?= pkg-config

CC ?= cc
RM ?= rm -f

cflags = -std=c99 -g -O2 -Wall -Wextra -Wpedantic -Wshadow \
		-Wno-implicit-fallthrough \
		-Werror=implicit-function-declaration -Werror=vla \
		$(shell $(PKGCONF) --cflags libbluray) $(CFLAGS)
ldflags = $(shell $(PKGCONF) --libs libbluray) $(LDFLAGS)

all:   bdinfo
clean:
	$(RM) bdinfo *.o
install: all
	$(INSTALL) -D     bdinfo   $(DESTDIR)$(PREFIX)/bin/bdinfo
	$(INSTALL) -Dm644 bdinfo.1 $(DESTDIR)$(PREFIX)/share/man/man1/bdinfo.1

bdinfo: src/bdinfo.c util.o
	$(CC) $(cflags) -o $@ $^ $(ldflags)

%.o: src/%.c src/%.h
	$(CC) -c $(cflags) -o $@ $<
