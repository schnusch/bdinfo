PREFIX ?= /usr/local

DETARBZ2   ?= tar -xj
DOWN       ?= curl -L
GPATCH     ?= patch
INSTALL    ?= install
PKG_CONFIG ?= pkg-config
STRIP      ?= strip

CFLAGS := -std=c99 -O0 -g -Wall -Wextra -Wpedantic \
			$(shell $(PKG_CONFIG) --cflags libbluray) $(CFLAGS)
LDFLAGS_BD := $(shell $(PKG_CONFIG) --libs libxml-2.0) -ldl
LDFLAGS    := $(shell $(PKG_CONFIG) --libs libbluray) $(LDFLAGS_BD) $(LDFLAGS)
LIBBLURAY = libbluray-0.9.2

BDINFO_OPT_DEPENDS    =
LIBBLURAY_STATUS_FILE = $(LIBBLURAY)/COPYING

# build flags

ifdef NO_CLIP_NAMES
	CFLAGS += -DNO_CLIP_NAMES
else
	BDINFO_OPT_DEPENDS += mpls.o $(LIBBLURAY)/.libs/libbluray.a
endif

ifdef NO_STRIP
	STRIP = @:
endif

all: bdinfo

clean:
	-$(RM) -fr $(LIBBLURAY) bdinfo *.o *.a

install: bdinfo
	$(INSTALL) -Dm 0755 -t $(DESTDIR)$(PREFIX)/bin bdinfo

bdinfo: bdinfo.c chapters.o mempool.o util.o $(BDINFO_OPT_DEPENDS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@

mpls.o: mpls.c mpls.h $(LIBBLURAY)/.libs/libbluray.a
	$(CC) -c -fPIC $(CFLAGS) -I$(LIBBLURAY)/src -o $@ $<

%.o: %.c %.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(LIBBLURAY_STATUS_FILE):
	$(DOWN) ftp://ftp.videolan.org/pub/videolan/libbluray/0.9.2/$(LIBBLURAY).tar.bz2 | $(DETARBZ2)
	touch $@

$(LIBBLURAY)/src/libbluray/bdnav/mpls_parse.c.patched: mpls.patch $(LIBBLURAY_STATUS_FILE)
	$(GPATCH) -d$(LIBBLURAY) -p2 < $<
	touch $@

$(LIBBLURAY)/Makefile: $(LIBBLURAY)/src/libbluray/bdnav/mpls_parse.c.patched
	cd $(LIBBLURAY) && env CFLAGS="-O0 -g -fvisibility=hidden" ./configure \
			--disable-shared --enable-static --disable-udf --disable-bdjava \
			--disable-doxygen-doc --disable-examples --without-freetype \
			--without-fontconfig

$(LIBBLURAY)/.libs/libbluray.a: $(LIBBLURAY)/Makefile
	$(MAKE) -C $(LIBBLURAY) libbluray.la