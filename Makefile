PREFIX     ?= /usr/local
PKG_CONFIG ?= pkg-config

CFLAGS  += -std=c99 -O2 -Wall -Wpedantic
CFLAGS  += $(shell $(PKG_CONFIG) --cflags libavformat libbluray)
LDFLAGS += $(shell $(PKG_CONFIG) --libs   libavformat libavutil libbluray)

DEPENDS = bdinfo.o bd.o mempool.o

bdinfo: $(DEPENDS)
	$(CC) -o bdinfo $(DEPENDS) $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

install: bdinfo
	install -D -m755 bdinfo $(DESTDIR)$(PREFIX)/bin/bdinfo

clean:
	-rm -f bdinfo $(DEPENDS)
