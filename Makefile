PREFIX ?= /usr/local
MANDIR ?= $(PREFIX)/share/man

DETARBZ2   ?= tar -xj
DOWN       ?= curl -L
GPATCH     ?= patch
GZ         ?= gzip -c
INSTALL    ?= install
OBJCOPY    ?= objcopy
PKG_CONFIG ?= pkg-config
STRIP      ?= strip

CFLAGS := -std=c99 -O2 -pipe -Wall -Wextra -Wpedantic \
			-Werror=implicit-function-declaration -Werror=vla \
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
	BDINFO_OPT_DEPENDS += mpls.o libbluray.a
endif

ifdef NO_STRIP
	STRIP = @:
endif

ifdef REQUIRE_LANGUAGES
	CFLAGS += -DREQUIRE_LANGS
	MAN_COMPRESS = sed -e 's,\\fR\[\\fILANGUAGES\\fR\]$$, LANGUAGES,' $< | $(GZ)
else
	MAN_COMPRESS = $(GZ) < $<
endif

all: bdinfo bdinfo.1.gz

clean:
	-$(RM) -fr $(LIBBLURAY) bdinfo bdinfo.1.gz libbluray.a *.o

install: bdinfo bdinfo.1.gz
	$(INSTALL) -Dm 0755 -t $(DESTDIR)$(PREFIX)/bin bdinfo
	$(INSTALL) -Dm 0644 bdinfo.1.gz $(DESTDIR)$(MANDIR)/man1/bdinfo.1.gz

bdinfo: bdinfo.c chapters.o mempool.o util.o $(BDINFO_OPT_DEPENDS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@

bdinfo.1.gz: bdinfo.1
	$(MAN_COMPRESS) > $@

mpls.o: mpls.c mpls.h libbluray.a
	$(CC) -c $(CFLAGS) -I$(LIBBLURAY)/src -o $@ $<

%.o: %.c %.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(LIBBLURAY_STATUS_FILE):
	$(DOWN) ftp://ftp.videolan.org/pub/videolan/libbluray/0.9.2/$(LIBBLURAY).tar.bz2 | $(DETARBZ2)
	touch $@

$(LIBBLURAY)/src/libbluray/bdnav/mpls_parse.c.patched: mpls.patch $(LIBBLURAY_STATUS_FILE)
	$(GPATCH) -d$(LIBBLURAY) -p2 < $<
	touch $@

$(LIBBLURAY)/Makefile: $(LIBBLURAY)/src/libbluray/bdnav/mpls_parse.c.patched
	cd $(LIBBLURAY) && env CFLAGS="-O2 -fvisibility=hidden" ./configure \
			--disable-shared --enable-static --disable-udf --disable-bdjava \
			--disable-doxygen-doc --disable-examples --without-freetype \
			--without-fontconfig

$(LIBBLURAY)/.libs/libbluray.a: $(LIBBLURAY)/Makefile
	$(MAKE) -C $(LIBBLURAY) libbluray.la

libbluray.a: $(LIBBLURAY)/.libs/libbluray.a objcopy.txt
	$(OBJCOPY) --redefine-syms=objcopy.txt $< $@
