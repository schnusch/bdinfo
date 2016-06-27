AM_CFLAGS = -std=c99 -O2 -pipe -Wall -Wextra -Wpedantic \
		-Werror=implicit-function-declaration -Werror=vla

bin_PROGRAMS = bdinfo
bdinfo_SOURCES = \
		src/bdinfo.c    \
		src/chapters.h  \
		src/chapters.c  \
		src/iso-639-2.h \
		src/mempool.h   \
		src/mempool.c   \
		src/util.h      \
		src/util.c
bdinfo_CFLAGS =
bdinfo_LDADD  =

MOSTLYCLEANFILES = bdinfo.1

if ENABLE_MANPAGE
if REQUIRE_LANGS
man1_MANS = bdinfo.1
else
man1_MANS = bdinfo.man
endif
endif

if WITH_SYSTEM_LIBBLURAY
bdinfo_CFLAGS += $(LIBBLURAY_CFLAGS)
bdinfo_LDADD  += $(LIBBLURAY_LIBS)
else
bdinfo_CFLAGS += -Isrc/libbluray/src $(LIBXML2_CFLAGS)
bdinfo_LDADD  += src/libbluray/.libs/libbluray.a -ldl $(LIBXML2_LIBS)
endif

if ENABLE_CLIPNAMES
if WITH_SYSTEM_LIBBLURAY
bdinfo_LDADD += src/libbluray.a
endif
bdinfo_SOURCES += src/mpls.h src/mpls.c
bdinfo_CFLAGS  += -Isrc/libbluray/src $(LIBXML2_CFLAGS)
bdinfo_LDADD   += -ldl $(LIBXML2_LIBS)
endif

src/libbluray/.libs/libbluray.a:
	$(MAKE) -C src/libbluray libbluray.la
src/libbluray.a: src/libbluray/.libs/libbluray.a objcopy.txt
	$(OBJCOPY) --redefine-syms=objcopy.txt $< $@

bdinfo.1: bdinfo.man bdinfo.man-required.patch
	patch -Ni bdinfo.man-required.patch $< -o $@
