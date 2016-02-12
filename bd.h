/* Copyright (C) 2016 Schnusch

   This file is part of bdinfo.

   libdecrypt is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   libdecrypt is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with libdecrypt.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdint.h>

#include <libbluray/bluray.h>

struct clip_map;

struct bluray_clip {
	char *name;
};

struct bluray_stream {
	uint16_t indx;
	uint16_t id;
	enum {
		BLURAY_STREAM_VIDEO    = 0,
		BLURAY_STREAM_AUDIO    = 1,
		BLURAY_STREAM_SUBTITLE = 2,
		BLURAY_STREAM_OTHER    = 3
	} type;
	enum {
		BLURAY_CODEC_PCM,
		BLURAY_CODEC_OTHER
	} codec;
	char lang[4];
};

struct bluray_title {
	uint32_t playlist;
	uint64_t duration;
	uint32_t clip_count;
	uint8_t  angle_count;
	uint8_t  angle;
	uint16_t stream_count;
	uint32_t chapter_count;
	uint64_t             *chapters;
	struct bluray_clip   *clips;
	struct bluray_stream *streams;
};

struct bluray_title_pile {
	struct clip_map *clips;
	uint32_t title_count;
	struct bluray_title titles[];
};

enum {
	TITLE_PILE_ALL      = 0x00,
	TITLE_PILE_PLAYLIST = 0x01,
	TITLE_PILE_ANGLE    = 0x02
};

#define LOG10TO2               3.3219280242919921875f
#define DECIMAL_BUFFER_LEN(t)  ((size_t)(sizeof(t) * CHAR_BIT / LOG10TO2) + 2)

/**
 * If \pplaylist is != -1 only return info for given playlist and angle,
 * otherwise return all playlists and angles with a minimum duration of
 * \pmin_duration.
 */
struct bluray_title_pile *get_bluray_title_pile(BLURAY *bd, const char *src,
		uint32_t min_duration, int flags, ...);

void free_bluray_title_pile(struct bluray_title_pile *pile);
