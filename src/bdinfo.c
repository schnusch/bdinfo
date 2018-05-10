/*
Copyright (C) 2016, 2018 Schnusch

This file is part of bdinfo.

bdinfo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

bdinfo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with bdinfo.  If not, see <http://www.gnu.org/licenses/>.
*/

#define BDINFO_VERSION  "?.?.?"
#define BLURAY_SPELLING "Blu-ray"

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libbluray/bluray.h>

#include "iso-639-2.h"
#include "util.h"

#define ANGLE_WILDCARD ((uint8_t)-1)


struct enum_map {
	int value;
	const char *name;
};

static const char *enum_map_search(const struct enum_map *map, int value)
{
	for(; map->name != NULL; map++)
		if(map->value == value)
			return map->name;
	return NULL;
}

static const char *get_stream_type(uint8_t type)
{
	static const struct enum_map types[] = {
		{BLURAY_STREAM_TYPE_VIDEO_MPEG1,             "MPEG-1"},
		{BLURAY_STREAM_TYPE_VIDEO_MPEG2,             "MPEG-2"},
		{BLURAY_STREAM_TYPE_VIDEO_H264,              "H.264/MPEG-4 AVC"},
		{BLURAY_STREAM_TYPE_VIDEO_VC1,               "VC-1/SMPTE 421M"},
		{BLURAY_STREAM_TYPE_AUDIO_MPEG1,             "MPEG-1"},
		{BLURAY_STREAM_TYPE_AUDIO_MPEG2,             "MPEG-2"},
		{BLURAY_STREAM_TYPE_AUDIO_LPCM,              "PCM"},
		{BLURAY_STREAM_TYPE_AUDIO_AC3,               "AC3"},
		{BLURAY_STREAM_TYPE_AUDIO_DTS,               "DTS"},
		{BLURAY_STREAM_TYPE_AUDIO_TRUHD,             "TrueHD"},
		{BLURAY_STREAM_TYPE_AUDIO_AC3PLUS,           "AC3+"},
		{BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY, "AC3+"},
		{BLURAY_STREAM_TYPE_AUDIO_DTSHD,             "DTS-HD"},
		{BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY,   "DTS-HD"},
		{BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER,      "DTS-HD MA"},
		{BLURAY_STREAM_TYPE_SUB_PG,                  "HDMV/PGS"},
		{BLURAY_STREAM_TYPE_SUB_IG,                  "HDMV/IGS"},
		{BLURAY_STREAM_TYPE_SUB_TEXT,                "HDMV/TEXT"},
		{0, NULL}
	};
	return enum_map_search(types, type);
}

static const char *get_video_format(uint8_t format)
{
	static const struct enum_map formats[] = {
		{BLURAY_VIDEO_FORMAT_480I,  "480i"},
		{BLURAY_VIDEO_FORMAT_576I,  "576i"},
		{BLURAY_VIDEO_FORMAT_480P,  "480p"},
		{BLURAY_VIDEO_FORMAT_1080I, "1080i"},
		{BLURAY_VIDEO_FORMAT_720P,  "720p"},
		{BLURAY_VIDEO_FORMAT_1080P, "1080p"},
		{BLURAY_VIDEO_FORMAT_576P,  "576p"},
		{0, NULL}
	};
	return enum_map_search(formats, format);
}

static const char *get_video_rate(uint8_t rate)
{
	static const struct enum_map rates[] = {
		{BLURAY_VIDEO_RATE_24000_1001, "23.976 fps"},
		{BLURAY_VIDEO_RATE_24,         "24 fps"},
		{BLURAY_VIDEO_RATE_25,         "25 fps"},
		{BLURAY_VIDEO_RATE_30000_1001, "29.97 fps"},
		{BLURAY_VIDEO_RATE_50,         "50 fps"},
		{BLURAY_VIDEO_RATE_60000_1001, "59.94 fps"},
		{0, NULL}
	};
	return enum_map_search(rates, rate);
}

static const char *get_aspect_ratio(uint8_t ratio)
{
	static const struct enum_map ratios[] = {
		{BLURAY_ASPECT_RATIO_4_3,  "4:3"},
		{BLURAY_ASPECT_RATIO_16_9, "16:9"},
		{0, NULL}
	};
	return enum_map_search(ratios, ratio);
}

static const char *get_audio_format(uint8_t format)
{
	static const struct enum_map formats[] = {
		{BLURAY_AUDIO_FORMAT_MONO,       "Mono"},
		{BLURAY_AUDIO_FORMAT_STEREO,     "Stereo"},
		{BLURAY_AUDIO_FORMAT_MULTI_CHAN, "Multi channel"},
		{BLURAY_AUDIO_FORMAT_COMBO,      "Combo"},
		{0, NULL}
	};
	return enum_map_search(formats, format);
}

static const char *get_audio_rate(uint8_t rate)
{
	static const struct enum_map rates[] = {
		{BLURAY_AUDIO_RATE_48,        "48 kHz"},
		{BLURAY_AUDIO_RATE_96,        "96 kHz"},
		{BLURAY_AUDIO_RATE_96_COMBO,  "96 kHz"},
		{BLURAY_AUDIO_RATE_192,       "192 kHz"},
		{BLURAY_AUDIO_RATE_192_COMBO, "192 kHz"},
		{0, NULL}
	};
	return enum_map_search(rates, rate);
}

struct playlist_selector {
	uint32_t playlist;
	uint8_t  angle;
};

#define FATALPUTC(c) \
		do \
		{ \
			if(fputc(c, stdout) == EOF) \
				return -1; \
		} \
		while(0)
#define FATALPUTS(s) \
		do \
		{ \
			if(fputs(s, stdout) == EOF) \
				return -1; \
		} \
		while(0)
#define FATALPRINTF(...) \
		do \
		{ \
			if(printf(__VA_ARGS__) < 0) \
				return -1; \
		} \
		while(0)
#define FATALDPRINTF(...) \
		do \
		{ \
			if(dprintf(__VA_ARGS__) < 0) \
				return -1; \
		} \
		while(0)

static int print_stream_extended(const BLURAY_STREAM_INFO *stream)
{
	FATALPRINTF("          - pid:          0x%04"PRIx16"\n", stream->pid);
	if(stream->lang[0])
		FATALPRINTF("            language:     %s\n", iso6392_bcode((const char *)stream->lang));
	const char *s;
	if((s = get_stream_type(stream->coding_type)))
		FATALPRINTF("            codec:        %s\n", s);
	return 0;
}

static int print_video_stream_extended(const BLURAY_STREAM_INFO *stream)
{
	if(print_stream_extended(stream) < 0)
		return -1;
	const char *s;
	if((s = get_aspect_ratio(stream->aspect)))
		FATALPRINTF("            aspect_ratio: %s\n", s);
	if((s = get_video_format(stream->format)))
		FATALPRINTF("            resolution:   %s\n", s);
	if((s = get_video_rate(stream->rate)))
		FATALPRINTF("            rate:         %s\n", s);
	return 0;
}

static int print_audio_stream_extended(const BLURAY_STREAM_INFO *stream)
{
	if(print_stream_extended(stream) < 0)
		return -1;
	const char *s;
	if((s = get_audio_format(stream->format)) != NULL)
		FATALPRINTF("            channels:     %s\n", s);
	if((s = get_audio_rate(stream->rate)) != NULL)
		FATALPRINTF("            rate:         %s\n", s);
	return 0;
}

static int print_streams(int (*print)(const BLURAY_STREAM_INFO *), ...)
{
	va_list ap;
	va_start(ap, print);
	int err = 0;
	const BLURAY_STREAM_INFO *streams;
	while((streams = va_arg(ap, void *)))
	{
		size_t numstreams = va_arg(ap, size_t);
		for(size_t i = 0; i < numstreams; i++)
		{
			if(!print)
			{
				if(printf("%s%s", i == 0 ? "" : ", ", streams[i].lang[0]
						? iso6392_bcode((const char *)streams[i].lang) : "und") < 0)
					goto err;
			}
			else if(print(streams + i) < 0)
				goto err;
		}
		continue;
	err:
		err = -1;
		break;
	}
	va_end(ap);
	if(!err && !print && fputs("]\n", stdout) == EOF)
		err = -1;
	return err;
}

static int print_all_streams(const BLURAY_CLIP_INFO *clip, int extended)
{
	int (*print_video)(const BLURAY_STREAM_INFO *) = extended ? print_video_stream_extended : NULL;
	int (*print_audio)(const BLURAY_STREAM_INFO *) = extended ? print_audio_stream_extended : NULL;
	int (*print_subs )(const BLURAY_STREAM_INFO *) = extended ? print_stream_extended       : NULL;
	int (*print_other)(const BLURAY_STREAM_INFO *) = extended ? print_stream_extended       : NULL;
	if(clip->video_stream_count == 0 && clip->sec_video_stream_count == 0)
		print_video = NULL;
	if(clip->audio_stream_count == 0 && clip->sec_audio_stream_count == 0)
		print_audio = NULL;
	if(clip->pg_stream_count == 0)
		print_subs = NULL;
	if(clip->ig_stream_count == 0)
		print_other = NULL;

	FATALPRINTF("    streams:\n        video:%s", print_video ? "\n" : "     [");
	if(print_streams(print_video, clip->video_streams, clip->video_stream_count,
			clip->sec_video_streams, clip->sec_video_stream_count, NULL) < 0)
		return -1;

	FATALPRINTF("        audio:%s", extended ? "\n" : "     [");
	if(print_streams(print_audio, clip->audio_streams, clip->audio_stream_count,
			clip->sec_audio_streams, clip->sec_audio_stream_count, NULL) < 0)
		return -1;

	FATALPRINTF("        subtitles:%s", extended ? "\n" : " [");
	if(print_streams(print_subs, clip->pg_streams, clip->pg_stream_count, NULL) < 0)
		return -1;

	FATALPRINTF("        other:%s", extended ? "\n" : "     [");
	if(print_streams(print_other, clip->ig_streams, clip->ig_stream_count, NULL) < 0)
		return -1;

	return 0;
}

static int print_clip(const BLURAY_CLIP_INFO *clip, int extended)
{
	FATALPRINTF("  - name: %s%s.m2ts\n", extended ? "    " : "", clip->clip_id);
	if(extended)
	{
		char timebuf[22];
		FATALPRINTF("    start:    %s\n", ticks2time(timebuf, clip->start_time));
		FATALPRINTF("    duration: %s\n", ticks2time(timebuf, clip->out_time - clip->in_time));
		FATALPRINTF("    skip:     %s\n", ticks2time(timebuf, clip->in_time));
	}
	if(print_all_streams(clip, extended) < 0)
		return -1;

	return 0;
}

static int print_clips(const BLURAY_CLIP_INFO *clips, size_t numclips, int extended)
{
	if(numclips == 0)
		FATALPUTS("clips:    []\n");
	else
	{
		if(numclips == 1)
			FATALPUTS("clips:\n");
		else
			FATALPRINTF("clips:    # %zu\n", numclips);

		for(size_t i = 0; i < numclips; i++)
			if(print_clip(clips + i, extended) < 0)
				return -1;
	}
	return 0;
}

static int print_title(const BLURAY_TITLE_INFO *title, int extended)
{
	char timebuf[22];
	FATALPRINTF("---\n"
			"playlist: %05"PRIu32".mpls\n"
			"angles:   %"PRIu8"\n"
			"duration: %s\n"
			"chapters: %"PRIu32"\n",
			title->playlist, title->angle_count,
			ticks2time(timebuf, title->duration), title->chapter_count);
	return print_clips(title->clips, title->clip_count, extended);
}

static int print_xml_chapters(const BLURAY_TITLE_INFO *title)
{
	static const char head[] =
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<Chapters>\n"
			"\t<EditionEntry>\n"
			"\t\t<EditionFlagHidden>0</EditionFlagHidden>\n"
			"\t\t<EditionFlagDefault>0</EditionFlagDefault>\n";
	static const char tail[] =
			"\t</EditionEntry>\n"
			"</Chapters>\n";

	const BLURAY_TITLE_CHAPTER *chapters = title->chapters;
	FATALPUTS(head);
	for(uint32_t i = 0; i < title->chapter_count; i++)
	{
		char timebuf[22];
		FATALPRINTF("\t\t<ChapterAtom>\n"
				"\t\t\t<ChapterTimeStart>%s</ChapterTimeStart>\n"
				"\t\t\t<ChapterFlagHidden>0</ChapterFlagHidden>\n"
				"\t\t\t<ChapterFlagEnabled>1</ChapterFlagEnabled>\n"
				"\t\t</ChapterAtom>\n",
				ticks2time(timebuf, chapters[i].start));
	}
	FATALPUTS(tail);
	return 0;
}

static int print_ff_chapters(const BLURAY_TITLE_INFO *title, int fd)
{
	const BLURAY_TITLE_CHAPTER *chapters = title->chapters;
	FATALDPRINTF(fd, ";FFMETADATA1\n");
	for(uint32_t i = 0; i < title->chapter_count; i++)
		FATALDPRINTF(fd, "[CHAPTER]\n"
				"TIMEBASE=1/90000\n"
				"START=%"PRIu64"\n"
				"END=%"PRIu64"\n",
				chapters[i].start,
				chapters[i].start + chapters[i].duration);
	return 0;
}

#undef FATALPUTC
#undef FATALPUTS
#undef FATALPRINTF

static char **argv_from_strs(char *s, size_t n)
{
	char **argv = NULL;
	int    argc = 0;
	while(n > 0)
	{
		if(!(argv = array_reserve(argv, argc, 1, sizeof(*argv)))) // FIXME realloc: NULL
			return NULL;
		argv[argc++] = s;
		char *s2 = memchr(s, '\0', n);
		if(s2)
			s2++, n -= s2 - s, s = s2;
		else
			n = 0;
	}
	if(!(argv = array_reserve(argv, argc, 1, sizeof(*argv)))) // FIXME realloc: NULL
		return NULL;
	argv[argc] = NULL;
	return argv;
}

struct strs_builder {
	char  *buf;
	size_t len;
	size_t end;
};

static char *strs_pushf(struct strs_builder *b, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int n;
	while(1)
	{
		va_list ap2;
		va_copy(ap2, ap);
		n = vsnprintf(b->buf + b->end, b->len - b->end, fmt, ap2);
		va_end(ap2);
		if(n < 0)
			return NULL;
		else if((size_t)n >= b->len - b->end)
		{
			if(!(b->buf = array_reserve(b->buf, b->end, n + 1, 1))) // FIXME realloc: NULL
				return NULL;
			b->len = b->end + n + 1;
		}
		else
			break;
	}
	va_end(ap);
	char *arg = b->buf + b->end;
	b->end += n + 1;
	return arg;
}

static char **generate_ffargv(const BLURAY_TITLE_INFO *title, char (*langs)[4],
		size_t numlangs, const char *src, const char *dst, int chapterfd,
		int transcode, int skip_ig)
{
	struct strs_builder b = {
		.buf = NULL,
		.len = 0,
		.end = 0
	};
	if(!strs_pushf(&b, "ffmpeg")
			|| !strs_pushf(&b, "-playlist") || !strs_pushf(&b, "%"PRIu32,   title->playlist)
//			|| !strs_pushf(&b, "-angle")    || !strs_pushf(&b, "%"PRIu8,    title->angle)
			|| !strs_pushf(&b, "-i")        || !strs_pushf(&b, "bluray:%s", src))
		goto error;
	if(title->chapter_count > 0)
	{
		const char *fmt = chapterfd == STDIN_FILENO ? "-" : "/dev/fd/%u";
		if(!strs_pushf(&b, "-i") || !strs_pushf(&b, fmt, chapterfd))
			goto error;
	}

	const BLURAY_STREAM_INFO *allstreams[] = {
		title->clips[0].video_streams,
		title->clips[0].sec_video_streams,
		title->clips[0].audio_streams,
		title->clips[0].sec_audio_streams,
		title->clips[0].pg_streams,
		title->clips[0].ig_streams
	};
	size_t numallstreams[] = {
		title->clips[0].video_stream_count,
		title->clips[0].sec_video_stream_count,
		title->clips[0].audio_stream_count,
		title->clips[0].sec_audio_stream_count,
		title->clips[0].pg_stream_count,
		skip_ig ? 0 : title->clips[0].ig_stream_count
	};
#define ITER_STREAMS(body) \
		for(size_t _i = 0, streamnum = 0; _i < 6; _i++) \
			for(size_t _j = 0; _j < numallstreams[_i]; _j++) { \
				const BLURAY_STREAM_INFO *stream = allstreams[_i] + _j; \
				const char *lang = stream->lang[0] ? iso6392_bcode((const char *)stream->lang) : NULL; \
				if(langs && stream->lang[0] && !bisect_contains(langs, lang ? lang : "", numlangs, sizeof(*langs), (compar_fn)strcmp)) \
					continue; \
				body \
				streamnum++; \
			}

	ITER_STREAMS(
		if(!strs_pushf(&b, "-map") || !strs_pushf(&b, "0:i:0x%04"PRIx16, stream->pid))
			goto error;
	)

	if(!strs_pushf(&b, "-c") || !strs_pushf(&b, "copy"))
		goto error;
	int flac = 0;
	ITER_STREAMS(
		if(stream->coding_type == BLURAY_STREAM_TYPE_AUDIO_LPCM || (transcode
				&& (stream->coding_type == BLURAY_STREAM_TYPE_AUDIO_TRUHD
						|| stream->coding_type == BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER)))
		{
			if(!strs_pushf(&b, "-c:%zu", streamnum) || !strs_pushf(&b, "flac"))
				goto error;
			flac = 1;
		}
	)
	if(flac)
		if(!strs_pushf(&b, "-compression_level") || !strs_pushf(&b, "12"))
			goto error;

	ITER_STREAMS(
		if(lang)
			if(!strs_pushf(&b, "-metadata:s:%zu", streamnum) || !strs_pushf(&b, "language=%s", lang))
				goto error;
	)

	if(title->chapter_count > 0)
		if(!strs_pushf(&b, "-map_chapters") || !strs_pushf(&b, "1"))
			goto error;

	if(!strs_pushf(&b, "%s", dst))
		goto error;

	char **argv = argv_from_strs(b.buf, b.end);
	if(!argv)
		goto error;
	return argv;

#undef ITER_STREAMS

error:
	// TODO errno
	free(b.buf);
	return NULL;
}

static int print_argv(char **argv)
{
	for(char **arg = argv; *arg;)
	{
		if(fputs(shell_escape(*arg), stdout) == EOF)
			return -1;
		if(*++arg && fputc(' ', stdout) == EOF)
			return -1;
	}
	return 0;
}

static int parse_playlist_arg(uint32_t *pl, uint8_t *an, const char *arg)
{
	unsigned long l;
	char *end;

	errno = 0;
	l = strtoul(arg, &end, 0);
	if(l > UINT32_MAX)
		l = ULONG_MAX, errno = ERANGE;
	if(l == ULONG_MAX && errno == ERANGE)
		return -1;
	*pl = l;
	*an = ANGLE_WILDCARD;

	if(!*end)
		return 0;
	else if(*end != ':')
		return -1;

	l = strtoul(arg, &end, 0);
	if(l > UINT8_MAX)
		l = ULONG_MAX, errno = ERANGE;
	if(l == ULONG_MAX && errno == ERANGE)
		return -1;
	*an = l;

	return *end ? -1 : 0;
}

static int cmp_playlist_selectors(const void *a_, const void *b_)
{
	const struct playlist_selector *a = a_;
	const struct playlist_selector *b = b_;
	int c = (a->playlist > b->playlist) - (a->playlist < b->playlist);
	if(c == 0)
	{
		if(a->angle == ANGLE_WILDCARD)
			c = -1;
		else if(b->angle == ANGLE_WILDCARD)
			c = 1;
		else
			c = (a->angle > b->angle) - (a->angle < b->angle);
	}
	return c;
}

static void clean_playlist_selectors(struct playlist_selector *playlists, size_t *numplaylists)
{
	if(*numplaylists == 0)
		return;
	qsort(playlists, *numplaylists, sizeof(*playlists), cmp_playlist_selectors);
	size_t off = 0;
	int wildcard = playlists[0].angle == ANGLE_WILDCARD;
	for(size_t src = 1; src < *numplaylists; src++)
	{
		if(playlists[src].playlist == playlists[src - 1].playlist)
		{
			if(wildcard || playlists[src].angle == playlists[src - 1].angle)
			{
				off++;
				continue;
			}
		}
		else
			wildcard = playlists[src].angle == ANGLE_WILDCARD;
		if(off > 0)
			playlists[src - off] = playlists[src];
	}
	*numplaylists -= off;
}

static int cmp_title_playlist(const void *a_, const void *b_)
{
	const BLURAY_TITLE_INFO *a = *(const void    **)a_;
	uint32_t                 b = *(const uint32_t *)b_;
	return (a->playlist > b) - (a->playlist < b);
}

static int cmp_title_infos(const void *a, const void *b_)
{
	const BLURAY_TITLE_INFO *b = *(const void **)b_;
	return cmp_title_playlist(a, &b->playlist);
}

int main(int argc, char **argv)
{
	int ok = 1;

	uint32_t min_duration = -1;
	int      filter_flags = TITLES_RELEVANT;
	int      operation    = 'l';
	enum {
		FLAG_TRANSCODE = 1,
		FLAG_SKIP_IG   = 2
	} flags = 0;

	struct playlist_selector *playlists = NULL;
	char                    (*langs)[4] = NULL;
	size_t numplaylists = 0;
	size_t numlangs     = 0;

	BLURAY             *bd     = NULL;
	BLURAY_TITLE_INFO **titles = NULL;
	char              **ffargv = NULL;
	size_t numtitles = 0;

	static const char optstring[] = "t:p:aicf::x::Lshv";
	static const struct option long_options[] = {
		{"time",        required_argument, NULL, 't'},
		{"playlist",    required_argument, NULL, 'p'},
		{"all",         no_argument,       NULL, 'a'},
//		{"multiple",    no_argument,       NULL, 'm'},
		{"info",        no_argument,       NULL, 'i'},
		{"chapters",    no_argument,       NULL, 'c'},
		{"ffmpeg",      optional_argument, NULL, 'f'},
		{"remux",       optional_argument, NULL, 'x'},
		{"lossless",    no_argument,       NULL, 'L'},
		{"skip-igs",    no_argument,       NULL, 's'},
		{"help",        no_argument,       NULL, 'h'},
		{"version",     no_argument,       NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	// parse arguments
	for(int c; (c = getopt_long(argc, argv, optstring, long_options, NULL)) != -1;)
		switch(c)
		{
			unsigned long long l;
			char    *end;
			uint32_t playlist;
			uint8_t  angle;
		case 'h':
			if(printf("Usage: %s [OPTION]... INPUT [OUTPUT]\n"
					"Get " BLURAY_SPELLING " info and extract tracks with ffmpeg.\n"
					"\n"
					"  -t, --time=DURATION        select all titles at least DURATION seconds long\n"
					"  -p, --playlist=PLAYLIST[:ANGLE]\n"
					"                             select playlist PLAYLIST and optionally only angle\n"
					"                             ANGLE\n"
					"  -a, --all                  do not omit duplicate titles\n"
					"  -i, --info                 print more detailed information\n"
					"  -c, --chapters             print XML chapters\n"
					"  -f, --ffmpeg[=LANGUAGES]   print ffmpeg call to extract all or only streams of\n"
					"                             given or undefined languages\n"
					"  -x, --remux[=LANGUAGES]    extract all or only streams of given or undefined\n"
					"                             languages with ffmpeg\n"
					"  -L, --lossless             transcode lossless audio tracks to FLAC\n"
					"  -s, --skip-igs             skip interactive graphic streams on extraction\n"
					"  -h, --help                 display this help and exit\n"
					"  -v, --version              output version information and exit\n",
					argv[0]) < 0)
				goto error_errno;
			return 0;
		case 'v':
			if(printf("%s " BDINFO_VERSION "\n"
					"Copyright (C) 2016 Schnusch\n"
					"License LGPLv3+: GNU LGPL version 3 or later <http://gnu.org/licenses/lgpl.html>.\n"
					"This is free software: you are free to change and redistribute it.\n"
					"There is NO WARRANTY, to the extent permitted by law.\n"
					"\n"
					"Written by Schnusch.\n", argv[0]) < 0)
				goto error_errno;
			return 0;
		case 't':
			errno = 0;
			l = strtoull(optarg, &end, 0);
			if(l > UINT64_MAX)
				l = ULLONG_MAX, errno = ERANGE;
			if((l == ULLONG_MAX && errno == ERANGE) || *end)
			{
				fprintf(stderr, "%s: Invalid duration %s\n", argv[0], optarg);
				goto error;
			}
			if(l < min_duration)
				min_duration = l;
			break;
		case 'p':
			if(parse_playlist_arg(&playlist, &angle, optarg) == -1)
			{
				fprintf(stderr, "%s: Invalid playlist %s\n", argv[0], optarg);
				goto error;
			}

			if(!(playlists = array_reserve(playlists, numplaylists, 1, sizeof(*playlists)))) // FIXME realloc: NULL
				goto error_errno;
			playlists[numplaylists].playlist = playlist;
			playlists[numplaylists++].angle  = angle;
			break;
		case 'a':
			filter_flags = 0;
			break;
		case 'L':
			flags |= FLAG_TRANSCODE;
			break;
		case 's':
			flags |= FLAG_SKIP_IG;
			break;
		case 'f':
		case 'x':
			if(optarg)
			{
				for(const char *lang, *next = optarg; (lang = iter_comma_list(&next, ','));)
				{
					// null-terminate lang
					char buf[4];
					if(next - lang == 3 && *next)
					{
						strncpy(buf, lang, 3);
						buf[3] = '\0';
						lang = buf;
					}

					if(!iso6392_known(lang))
					{
						fprintf(stderr, "%s: Unknown ISO 639-2 language requested:"
								" %s\n", argv[0], lang);
						continue;
					}

					lang = iso6392_bcode(lang);
					if(!(langs = array_reserve(langs, numlangs, 1, sizeof(*langs)))) // FIXME realloc: NULL
						goto error_errno;
					strcpy(langs[numlangs++], lang);
				}
				qsort(langs, numlangs, sizeof(*langs), (compar_fn)strcmp);
			}
		case 'i':
		case 'c':
			operation = c;
			break;
		}

	const char *src = argv[optind++];
	if(!src)
	{
		fprintf(stderr, "%s: No "BLURAY_SPELLING" given\n", argv[0]);
		return 2;
	}

	const char *dst = argv[optind];
	if(operation == 'f' || operation == 'x')
		optind++;
	if(optind > argc)
	{
		fprintf(stderr, "%s: no destination file given\n", argv[0]);
		goto error;
	}
	else if(optind < argc)
	{
		fprintf(stderr, "%s: trailing arguments\n", argv[0]);
		goto error;
	}

	if(numplaylists > 0)
		clean_playlist_selectors(playlists, &numplaylists);
	else if(min_duration == (uint32_t)-1)
		min_duration = 0;

	// open bluray
	bd = bd_open(src, NULL);
	if(!bd)
		goto error_libbluray;

	// get BLURAY_TITLE_INFOs by duration
	if(min_duration != (uint32_t)-1)
	{
		uint32_t n = bd_get_titles(bd, filter_flags, min_duration);
		if(!(titles = array_reserve(titles, numtitles, n, sizeof(*titles)))) // FIXME realloc: NULL
			goto error_errno;
		for(uint32_t i = 0; i < n; i++)
		{
			BLURAY_TITLE_INFO *title = bd_get_title_info(bd, i, 0);
			if(!title)
				goto error_libbluray;
			titles[numtitles++] = title;
		}
	}
	qsort(titles, numtitles, sizeof(*titles), cmp_title_infos);

	// get BLURAY_TITLE_INFOs by playlist selectors
	for(size_t i = 0; i < numplaylists; i++)
	{
		uint32_t playlist = playlists[i].playlist;

		// skip playlist if already selected by time
		size_t j = bisect_left(titles, &playlist, numtitles, sizeof(*titles), cmp_title_playlist);
		if(j < numtitles && titles[j]->playlist == playlist)
			continue;

		BLURAY_TITLE_INFO *title = bd_get_playlist_info(bd, playlist, 0);
		if(!title)
			goto error_libbluray;

		if(!(titles = array_reserve(titles, numtitles, 1, sizeof(*titles)))) // FIXME realloc: NULL
		{
			int errnum = errno;
			bd_free_title_info(title);
			errno = errnum;
			goto error_errno;
		}
		titles[numtitles++] = title;
	}

	if(numtitles == 0)
	{
		fprintf(stderr, "%s: No title selected\n", argv[0]);
		goto error;
	}

	if(operation == 'l' || operation == 'i')
	{
		for(size_t i = 0; i < numtitles; i++)
			if(print_title(titles[i], operation == 'i') < 0)
				goto error_errno;
		if(fputs("...\n", stdout) == EOF)
			goto error_errno;
	}
	else
	{
		if(numtitles > 1)
		{
			const char short_opt[2] = {operation, '\0'};
			const char *long_opt = NULL;
			for(const struct option *opt = long_options; opt->name; opt++)
				if(opt->val == operation)
				{
					long_opt = opt->name;
					break;
				}
			fprintf(stderr, "%s: %s%s requires a single title (%zu selected)\n",
					argv[0], long_opt ? "--" : "-", long_opt ? long_opt : short_opt, numtitles);
			goto error;
		}

		BLURAY_TITLE_INFO *title = titles[0];
		if(operation == 'c')
		{
			if(print_xml_chapters(title) == -1)
				goto error_errno;
		}
		else
		{
			int tempfd = STDIN_FILENO;
			if(operation == 'x' && title->chapter_count > 0)
			{
				char template[] = "/tmp/chapterinfo.XXXXXX";
				tempfd = mkstemp(template);
				if(tempfd < 0)
					goto error_errno;
				if (unlink(template) < 0)
					goto error_errno;

				if (print_ff_chapters(title, tempfd) < 0)
					goto error_errno;

				if (lseek(tempfd, 0, SEEK_SET) < 0)
					goto error_errno;
			}

			ffargv = generate_ffargv(title, langs, numlangs, src, dst, tempfd,
					flags & FLAG_TRANSCODE, flags & FLAG_SKIP_IG);
			if(!ffargv)
			{
				if(operation == 'x')
				{
					int errbak = errno;
					close(tempfd);
					errno = errbak;
				}
				goto error_errno;
			}

			if(operation == 'f')
			{
				if(print_argv(ffargv) < 0)
					goto error_errno;
				if(title->chapter_count > 0)
					if(fputs(" << EOF\n", stdout) == EOF
							|| print_ff_chapters(title, STDOUT_FILENO) < 0
							|| fputs("EOF", stdout) == EOF)
						goto error_errno;
				if(fputc('\n', stdout) == EOF)
					goto error_errno;
			}
			else
			{
				execvp("ffmpeg", ffargv);
				int errbak = errno;
				close(tempfd);
				errno = errbak;
				goto error_errno;
			}
		}
	}

	if(0)
	{
	error_errno:
		perror(argv[0]);
		goto error;
	error_libbluray:
		fprintf(stderr, "%s: Error in %s\n", argv[0], src);
	error:
		ok = 0;
	}
	for(size_t i = 0; i < numtitles; i++)
		bd_free_title_info(titles[i]);
	free(titles);
	if(ffargv)
		free(ffargv[0]);
	free(ffargv);
	free(langs);
	if(bd)
		bd_close(bd);

	return !ok;
}
