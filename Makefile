PKG_CONFIG ?= pkg-config
STRIP      ?= strip

CFLAGS := -std=c99 -O2 -Wall -Wextra -Wpedantic \
			$(shell $(PKG_CONFIG) --cflags libavformat libavutil libbluray) \
			$(CFLAGS)
LDFLAGS := $(shell $(PKG_CONFIG) --libs libavformat libavutil libbluray) \
			$(LDFLAGS)

ifdef NO_CLIP_NAMES
	CFLAGS += -DNO_CLIP_NAMES
endif
ifdef NO_LIBAVFORMAT
	CFLAGS += -DNO_LIBAVFORMAT
endif
ifdef NO_STRIP
	STRIP = @:
endif

bdinfo: bdinfo.c bd.o mempool.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@

clean:
	-$(RM) bdinfo *.o

%.o: %.c %.h
	$(CC) -c $(CFLAGS) -o $@ $<
