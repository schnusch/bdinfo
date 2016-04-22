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

#define BLURAY_SPELLING "Blu-ray"
#define VERSION         "1.0.1"

#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libbluray/bluray.h>

#include "chapters.h"
#include "mempool.h"
#ifndef NO_CLIP_NAMES
	#include "mpls.h"
#endif
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

const char *get_stream_type(uint8_t type)
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
			{0, NULL}};
	return enum_map_search(types, type);
}

const char *get_video_format(uint8_t format)
{
	static const struct enum_map formats[] = {
			{BLURAY_VIDEO_FORMAT_480I,  "480i"},
			{BLURAY_VIDEO_FORMAT_576I,  "576i"},
			{BLURAY_VIDEO_FORMAT_480P,  "480p"},
			{BLURAY_VIDEO_FORMAT_1080I, "1080i"},
			{BLURAY_VIDEO_FORMAT_720P,  "720p"},
			{BLURAY_VIDEO_FORMAT_1080P, "1080p"},
			{BLURAY_VIDEO_FORMAT_576P,  "576p"},
			{0, NULL}};
	return enum_map_search(formats, format);
}

const char *get_video_rate(uint8_t rate)
{
	static const struct enum_map rates[] = {
			{BLURAY_VIDEO_RATE_24000_1001, "23.976 fps"},
			{BLURAY_VIDEO_RATE_24,         "24 fps"},
			{BLURAY_VIDEO_RATE_25,         "25 fps"},
			{BLURAY_VIDEO_RATE_30000_1001, "29.97 fps"},
			{BLURAY_VIDEO_RATE_50,         "50 fps"},
			{BLURAY_VIDEO_RATE_60000_1001, "59.94 fps"},
			{0, NULL}};
	return enum_map_search(rates, rate);
}

const char *get_aspect_ratio(uint8_t ratio)
{
	static const struct enum_map ratios[] = {
			{BLURAY_ASPECT_RATIO_4_3,  "4:3"},
			{BLURAY_ASPECT_RATIO_16_9, "16:9"},
			{0, NULL}};
	return enum_map_search(ratios, ratio);
}

const char *get_audio_format(uint8_t format)
{
	static const struct enum_map formats[] = {
			{BLURAY_AUDIO_FORMAT_MONO,       "Mono"},
			{BLURAY_AUDIO_FORMAT_STEREO,     "Stereo"},
			{BLURAY_AUDIO_FORMAT_MULTI_CHAN, "Multi channel"},
			{BLURAY_AUDIO_FORMAT_COMBO,      "Combo"},
			{0, NULL}};
	return enum_map_search(formats, format);
}

const char *get_audio_rate(uint8_t rate)
{
	static const struct enum_map rates[] = {
			{BLURAY_AUDIO_RATE_48,        "48 kHz"},
			{BLURAY_AUDIO_RATE_96,        "96 kHz"},
			{BLURAY_AUDIO_RATE_96_COMBO,  "96 kHz"},
			{BLURAY_AUDIO_RATE_192,       "192 kHz"},
			{BLURAY_AUDIO_RATE_192_COMBO, "192 kHz"},
			{0, NULL}};
	return enum_map_search(rates, rate);
}

struct iso6392_map {
	const char *prefered;
	const char *possible;
};

static const struct iso6392_map langdoubles[] = {
		{"tib", "bod"},
		{"cze", "ces"},
		{"wel", "cym"},
		{"ger", "deu"},
		{"baq", "eus"},
		{"gre", "ell"},
		{"per", "fas"},
		{"fre", "fra"},
		{"arm", "hye"},
		{"ice", "isl"},
		{"geo", "kat"},
		{"mao", "mri"},
		{"may", "msa"},
		{"bur", "mya"},
		{"dut", "nld"},
		{"rum", "ron"},
		{"slo", "slk"},
		{"alb", "sqi"},
		{"chi", "zho"},
		{NULL,  NULL}};

const char *prefered_iso6392(const char *lang)
{
	size_t l = 0;
	size_t h = sizeof(langdoubles) / sizeof(langdoubles[0]) - 1;
	while(l < h)
	{
		size_t m = (l + h) / 2;
		if(strncmp(langdoubles[m].possible, lang, 3) < 0)
			l = m + 1;
		else
			h = m;
	}
	if(strncmp(langdoubles[l].possible, lang, 3) == 0)
		return langdoubles[l].prefered;
	else
		return lang;
}

struct pllist {
	uint32_t pl;
	uint8_t an;
	struct pllist *next;
};

struct tilist {
	BLURAY_TITLE_INFO *ti;
	uint32_t *clips; // TODO
	uint8_t an[sizeof(void *) / sizeof(uint8_t)];
	struct tilist *next;
};

#define FATALPUTC(c) \
		do \
		{ \
			if(fputc((c), stdout) == EOF) \
				return -1; \
		} \
		while(0)
#define FATALPUTS(s) \
		do \
		{ \
			if(fputs((s), stdout) == EOF) \
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

#ifndef NO_CLIP_NAMES
static int iter_angles(struct tilist *til,
		int (*callback)(BLURAY_TITLE_INFO *, uint8_t, void *), void *data)
{
	if(til->an[0] == ANGLE_WILDCARD)
	{
		for(uint8_t ia = 0; ia < til->ti->angle_count; ia++)
			if(callback(til->ti, ia, data) == -1)
				return -1;
		til = til->next;
	}
	else
	{
		for(; til; til = til->next)
			for(uint8_t ia = 0; ia < sizeof(til->an) / sizeof(til->an[0]); ia++)
			{
				if(til->an[ia] == ANGLE_WILDCARD)
					goto schnorpf;
				if(callback(til->ti, til->an[ia], data) == -1)
					return -1;
			}
	schnorpf:
		;
	}
	return 0;
}

struct clip_payload {
	uint32_t *clip;
	uint32_t  tmp;
	bool      first;
};

static int print_clip_names(BLURAY_TITLE_INFO *ti, uint8_t an, void *data_)
{
	struct clip_payload *data = data_;
	FATALPRINTF("%s%" PRIu8 ": %05" PRIu32 ".m2ts", data->first ? "{" : ", ",
			an, *data->clip);
	data->clip += ti->angle_count;
	data->first = false;
	return 0;
}

static int compare_clip_names(BLURAY_TITLE_INFO *ti,
		uint8_t an __attribute__((unused)), void *data_)
{
	struct clip_payload *data = data_;
	if(data->first)
	{
		data->first = false;
		data->tmp   = *data->clip;
	}
	else if(*data->clip != data->tmp)
		return -1;
	data->clip += ti->clip_count;
	return 0;
}
#endif

static int print_streams(BLURAY_CLIP_INFO *cl, bool extended)
{
	if(cl->video_stream_count > 0 || cl->audio_stream_count > 0
			|| cl->pg_stream_count > 0 || cl->ig_stream_count > 0
			|| cl->sec_video_stream_count > 0
			|| cl->sec_audio_stream_count > 0)
	{
		FATALPUTS("streams:\n");
		// video
		if(cl->video_stream_count > 0 || cl->sec_video_stream_count)
		{
			bool first = true;
			FATALPUTS("        video:");
			if(!extended && cl->pg_stream_count > 0)
				FATALPUTS("   ");
			BLURAY_STREAM_INFO *sts = cl->video_streams;
			uint8_t ns = cl->video_stream_count;
			goto video_start;
			do
			{
				sts = cl->sec_video_streams;
				ns  = cl->sec_video_stream_count;
			video_start:
				for(uint8_t is = 0; is < ns; is++)
				{
					BLURAY_STREAM_INFO *st = &sts[is];
					if(extended)
					{
						FATALPRINTF("\n          - pid:          0x%04" PRIx16,
								st->pid);
						if(st->lang[0] != '\0')
							FATALPRINTF("\n            language:     %s", prefered_iso6392((char *)st->lang));
						const char *s;
						if((s = get_stream_type(st->coding_type)) != NULL)
							FATALPRINTF("\n            codec:        %s", s);
						if((s = get_video_format(st->format)) != NULL)
							FATALPRINTF("\n            resolution:   %s", s);
						if((s = get_aspect_ratio(st->aspect)) != NULL)
							FATALPRINTF("\n            aspect_ratio: %s", s);
						if((s = get_video_rate(st->rate)) != NULL)
							FATALPRINTF("\n            rate:         %s", s);
					}
					else
					{
						FATALPRINTF("%s%s", first ? " [" : ", ",
								st->lang[0] == '\0' ? "und" : prefered_iso6392((char *)st->lang));
						first = false;
					}
				}
			}
			while(sts != cl->sec_video_streams);
			if(!extended)
				FATALPUTC(']');
			FATALPUTC('\n');
		}
		// audio
		if(cl->audio_stream_count > 0 || cl->sec_audio_stream_count)
		{
			bool first = true;
			FATALPUTS("        audio:");
			if(!extended && cl->pg_stream_count > 0)
				FATALPUTS("   ");
			BLURAY_STREAM_INFO *sts = cl->audio_streams;
			uint8_t ns = cl->audio_stream_count;
			goto audio_start;
			do
			{
				sts = cl->sec_audio_streams;
				ns  = cl->sec_audio_stream_count;
			audio_start:
				for(uint8_t is = 0; is < ns; is++)
				{
					BLURAY_STREAM_INFO *st = &sts[is];
					if(extended)
					{
						FATALPRINTF("\n          - pid:      0x%04" PRIx16,
								st->pid);
						if(st->lang[0] != '\0')
							FATALPRINTF("\n            language: %s", prefered_iso6392((char *)st->lang));
						const char *s;
						if((s = get_stream_type(st->coding_type)) != NULL)
							FATALPRINTF("\n            codec:    %s", s);
						if((s = get_audio_format(st->format)) != NULL)
							FATALPRINTF("\n            channels: %s", s);
						if((s = get_audio_rate(st->rate)) != NULL)
							FATALPRINTF("\n            rate:     %s", s);
					}
					else
					{
						FATALPRINTF("%s%s", first ? " [" : ", ",
								st->lang[0] == '\0' ? "und" : prefered_iso6392((char *)st->lang));
						first = false;
					}
				}
			}
			while(sts != cl->sec_audio_streams);
			if(!extended)
				FATALPUTC(']');
			FATALPUTC('\n');
		}
		// subtitles
		if(cl->pg_stream_count > 0)
		{
			bool first = true;
			FATALPUTS("        subtitle:");
			for(uint8_t is = 0; is < cl->pg_stream_count; is++)
			{
				BLURAY_STREAM_INFO *st = &cl->pg_streams[is];
				if(extended)
				{
					FATALPRINTF("\n          - pid:      0x%04" PRIx16, st->pid);
					if(st->lang[0] != '\0')
						FATALPRINTF("\n            language: %s", prefered_iso6392((char *)st->lang));
					const char *s;
					if((s = get_stream_type(st->coding_type)) != NULL)
						FATALPRINTF("\n            codec:    %s", s);
				}
				else
				{
					FATALPRINTF("%s%s", first ? " [" : ", ",
							st->lang[0] == '\0' ? "und" : prefered_iso6392((char *)st->lang));
					first = false;
				}
			}
			if(!extended)
				FATALPUTC(']');
			FATALPUTC('\n');
		}
		// other
		if(cl->ig_stream_count > 0)
		{
			bool first = true;
			FATALPUTS("        other:");
			if(!extended && cl->pg_stream_count > 0)
				FATALPUTS("   ");
			for(uint8_t is = 0; is < cl->ig_stream_count; is++)
			{
				BLURAY_STREAM_INFO *st = &cl->ig_streams[is];
				if(extended)
				{
					FATALPRINTF("\n          - pid:      0x%04" PRIx16,
							st->pid);
					if(st->lang[0] != '\0')
						FATALPRINTF("\n            language: %s", prefered_iso6392((char *)st->lang));
					const char *s;
					if((s = get_stream_type(st->coding_type)) != NULL)
						FATALPRINTF("\n            codec:    %s", s);
				}
				else
				{
					FATALPRINTF("%s%s", first ? " [" : ", ",
							st->lang[0] == '\0' ? "und" : prefered_iso6392((char *)st->lang));
					first = false;
				}
			}
			if(!extended)
				FATALPUTC(']');
			FATALPUTC('\n');
		}
	}
	return 0;
}

/**
 * list available tracks
 */
int list_titles(struct tilist *til, bool extended, bool all_the_same)
{
	while(til)
	{
		BLURAY_TITLE_INFO *ti = til->ti;

		FATALPRINTF("---\n"
				"playlist: %05" PRIu32 ".mlps\n"
				"angles:   %" PRIu8 "\n"
				"duration: %s\n"
				"chapters: %" PRIu32 "\n"
				"clips:", ti->playlist, ti->angle_count,
				ticks2time(ti->duration), ti->chapter_count);
		if(ti->clip_count == 0)
			FATALPUTS("    []");
		if(ti->clip_count > 1)
			FATALPRINTF("  # %" PRIu32, ti->clip_count);
		FATALPUTC('\n');

		for(uint32_t ic = 0; ic < ti->clip_count; ic++)
		{
			BLURAY_CLIP_INFO *cl = &ti->clips[ic];
			FATALPUTS("  - ");
#ifndef NO_CLIP_NAMES
			if(all_the_same)
			{
				// compact clip names
				struct clip_payload payload;
				payload.clip  = &til->clips[ic];
				payload.first = true;
				all_the_same = iter_angles(til, &compare_clip_names, &payload) == 0;
			}
			FATALPRINTF("name%s: ", all_the_same ? "" : "s");
			if(all_the_same)
				FATALPRINTF("%05" PRIu32 ".m2ts", til->clips[ic]);
			else
			{
				struct clip_payload payload;
				payload.clip  = &til->clips[ic];
				payload.first = true;
				if(iter_angles(til, &print_clip_names, &payload) == -1)
					return -1;
				FATALPUTC('}');
			}
			FATALPUTS("\n    ");
#endif
			if(extended)
			{
				FATALPRINTF("start:    %s\n    ", ticks2time(cl->start_time));
				FATALPRINTF("duration: %s\n    ", ticks2time(cl->out_time - cl->in_time));
				FATALPRINTF("skip:     %s\n    ", ticks2time(cl->in_time));
			}
			if(print_streams(cl, extended) == -1)
				return -1;
		}

		for(; til && til->ti == ti; til = til->next);
	}
	return 0;
}

#undef FATALPUTC
#undef FATALPUTS
#undef FATALPRINTF

static void iter_streams(BLURAY_CLIP_INFO *cl,
		void (*callback)(BLURAY_STREAM_INFO *, void *), void *data)
{
	BLURAY_STREAM_INFO *base = cl->video_streams;
	uint8_t ns = cl->video_stream_count;
	while(1)
	{
		for(uint8_t is = 0; is < ns; is++)
			callback(&base[is], data);

		if(base == cl->video_streams)
		{
			base = cl->sec_video_streams;
			ns   = cl->sec_video_stream_count;
		}
		else if(base == cl->sec_video_streams)
		{
			base = cl->audio_streams;
			ns   = cl->audio_stream_count;
		}
		else if(base == cl->audio_streams)
		{
			base = cl->sec_audio_streams;
			ns   = cl->sec_audio_stream_count;
		}
		else if(base == cl->sec_audio_streams)
		{
			base = cl->pg_streams;
			ns   = cl->pg_stream_count;
		}
		else if(base == cl->pg_streams)
		{
			base = cl->ig_streams;
			ns   = cl->ig_stream_count;
		}
		else
			break;
	}
}

struct ffargv_count {
	const char **langs;
	size_t   argc;
	uint16_t maps;
	uint16_t metas;
	uint8_t  convs;
};

struct ffarg {
	const char **langs;
	const char **argp1;
	char        *argp2;
	uint16_t     i;
};

static void ffargv_count(BLURAY_STREAM_INFO *st, void *data)
{
	struct ffargv_count *cnt = data;
	for(const char **lang = cnt->langs; *lang; lang++)
		if(st->lang[0] == '\0' || strncmp((char *)st->lang, *lang, 4) == 0)
		{
			cnt->maps++;
			cnt->metas += (st->lang[0] != '\0');
			if(st->coding_type == BLURAY_STREAM_TYPE_AUDIO_LPCM)
			{
				cnt->convs += 3 + DECIMAL_BUFFER_LEN(uint16_t);
				cnt->argc += 4;
			}
			break;
		}
}

#define ARGV_APPENDF(argp1, argp2, format, ...) \
		((argp2) += sprintf((char *)(*(argp1)++ = (argp2)), \
				format, __VA_ARGS__) + 1)

static void ffargv_fill(BLURAY_STREAM_INFO *st, void *data)
{
	struct ffarg *ff = data;
	for(const char **lang = ff->langs; *lang; lang++)
		if(st->lang[0] == '\0' || strncmp((char *)st->lang, *lang, 4) == 0)
		{
			*ff->argp1++ = "-map";
			ARGV_APPENDF(ff->argp1, ff->argp2, "0:i:0x%04" PRIx16, st->pid);
			if(st->lang[0] != '\0')
			{
				ARGV_APPENDF(ff->argp1, ff->argp2, "-metadata:s:%" PRIu16, ff->i);
				ARGV_APPENDF(ff->argp1, ff->argp2, "language=%s",  *lang);
			}
			if(st->coding_type == BLURAY_STREAM_TYPE_AUDIO_LPCM)
			{
				// FIXME keep pcm
				ARGV_APPENDF(ff->argp1, ff->argp2, "-c:%" PRIu16, ff->i);
				*ff->argp1++ = "flac";
				*ff->argp1++ = "-compression_level";
				*ff->argp1++ = "12";
			}
			ff->i++;
			break;
		}
}

const char **generate_ffargv(struct tilist *til, const char **langs,
		const char *src, const char *dst, int chapterfd)
{
	/*
	ffmpeg -playlist %d -angle %d -i bluray:%s [-i /proc/self/fd/%d]
			-c copy [[-map 0:%c:%d] [-metadata:s:%d language=%s]]...
			[-map_chapters 1] %s
	*/
	BLURAY_TITLE_INFO *ti = til->ti;
	size_t ffargc = 10;
	size_t ffargn = (DECIMAL_BUFFER_LEN(uint32_t) + 1)
			+ (DECIMAL_BUFFER_LEN(uint8_t) + 1) + (7 + strlen(src) + 1);
	if(chapterfd != -1)
	{
		ffargc += 4;
		if(chapterfd != 0)
			ffargn += 14 + DECIMAL_BUFFER_LEN(int) + 1;
	}

	// count mappings
	struct ffargv_count cnt;
	cnt.langs = langs;
	cnt.argc  = 0;
	cnt.maps  = 0;
	cnt.metas = 0;
	cnt.convs = 0;
	iter_streams(&ti->clips[0], &ffargv_count, &cnt);
	ffargc += 2 * (cnt.maps + cnt.metas) + cnt.argc;
	ffargn += cnt.maps * 11
			+ cnt.metas * (12 + DECIMAL_BUFFER_LEN(uint16_t) + 1 + 12 + 1)
			+ cnt.convs;

	const char **ffargv = malloc((ffargc + 1) * sizeof(char *) + ffargn);
	if(ffargv == NULL)
		return NULL;

	// fill argv
	struct ffarg ff;
	ff.langs = langs;
	ff.argp1 = ffargv;
	ff.argp2 = (char *)(ffargv + ffargc + 1);

	*ff.argp1++ = "ffmpeg";
	*ff.argp1++ = "-playlist";
	ARGV_APPENDF(ff.argp1, ff.argp2, "%" PRIu32, ti->playlist);
	*ff.argp1++ = "-angle";
	ARGV_APPENDF(ff.argp1, ff.argp2, "%" PRIu8,
			til->an[0] == ANGLE_WILDCARD ? 0 : til->an[0]);
	*ff.argp1++ = "-i";
	ff.argp2 = stpcpy(stpcpy((char *)(*ff.argp1++ = ff.argp2),
			"bluray:"), src) + 1;
	if(chapterfd != -1)
	{
		*ff.argp1++ = "-i";
		if(chapterfd == 0)
			*ff.argp1++ = "-";
		else
			ARGV_APPENDF(ff.argp1, ff.argp2, "/proc/self/fd/%d", chapterfd);
	}
	*ff.argp1++ = "-c";
	*ff.argp1++ = "copy";
	// argvify mappings
	ff.i = 0;
	iter_streams(&ti->clips[0], &ffargv_fill, &ff);
	if(chapterfd != -1)
	{
		*ff.argp1++ = "-map_chapters";
		*ff.argp1++ = "1";
	}
	*ff.argp1++ = dst;
	*ff.argp1 = NULL;

	return ffargv;
}

int start_ffmpeg_chapter_process(BLURAY *bd, BLURAY_TITLE_INFO *ti, int fds[2],
		const char *argv0)
{
	// TODO use memfd_create to avoid nasty forking business

	pid_t child = fork();
	if(child == -1)
	{
		close(fds[1]);
	error2:
		close(fds[0]);
		perror(argv0);
		return -1;
	}
	else if(child == 0)
	{
		// 1st child process
		close(fds[0]);

		if(dup2(fds[1], STDOUT_FILENO) == -1)
		{
			close(fds[1]);
		error3:
			bd_free_title_info(ti);
			bd_close(bd);
			perror(argv0);
			_exit(1);
		}
		close(fds[1]);

		// fork again
		child = fork();
		if(child == -1)
			goto error3;
		else if(child != 0)
		{
			// exit 1st child process
			fork();
			close(fds[1]);
			_exit(0);
		}

		// 2nd child process
		int e = print_ff_chapters(ti) == -1 || fflush(stdout) == EOF;
		if(e)
			fprintf(stderr, "%s: Failed to write chapters\n", argv0);

		bd_free_title_info(ti);
		bd_close(bd);

		_exit(e);
	}

	// parent process
	close(fds[1]);

	// wait for 1st child to exit
	int status;
	waitpid(child, &status, 0);
	if(WEXITSTATUS(status) != 0)
		goto error2;

	return 0;
}

static int parse_playlist_arg(uint32_t *pl, uint8_t *an, char *arg)
{
	char *s = strchr(arg, ':');
	if(s == NULL)
		*an = ANGLE_WILDCARD;
	else
	{
		if(*(++s) == '\0' || dec2uint(an, sizeof(uint8_t), s) == -1
				|| *an == ANGLE_WILDCARD)
			return -1;
		*(--s) = '\0';
	}
	if(dec2uint(pl, sizeof(uint32_t), arg) == -1)
	{
		if(s != NULL)
			*s = ':';
		return -1;
	}
	return 0;
}

static struct tilist *tilist_create(struct tilist **ptil, BLURAY *bd,
		BLURAY_TITLE_INFO *ti, uint32_t *clips, mempool_t *listpool,
		const char *argv0)
{
	struct tilist *til = mempool_alloc(listpool);
	if(til == NULL)
	{
	error:
		bd_free_title_info(ti);
		return NULL;
	}
	if(clips == NULL)
	{
		clips = malloc(ti->angle_count * ti->clip_count * sizeof(uint32_t));
		if(clips == NULL)
			goto error;
#ifndef NO_CLIP_NAMES
		// parse clip names
		char mpls_path[25];
		errno = 0;
		if(snprintf(mpls_path, 25, "BDMV/PLAYLIST/%05" PRIu32 ".mpls",
				ti->playlist) != 24)
		{
			if(errno == 0)
				fprintf(stderr, "%s: Failed to parse BDMV/PLAYLIST/%05" PRIu32
						".mpls\n", argv0, ti->playlist);
			goto error;
		}
		void   *mpls_buf;
		int64_t n;
		if(!bd_read_file(bd, mpls_path, &mpls_buf, &n))
			goto error;
		if(mpls_memparse(clips, mpls_buf, (size_t)n, ti->angle_count,
				ti->clip_count) == -1)
			goto error;
#endif
	}
	til->ti    = ti;
	til->clips = clips;
	til->next  = *ptil;
	*ptil = til;
	return til;
}

static int tilist_add_by_duration(struct tilist **ptil_first, BLURAY *bd,
		int filter_flags, uint32_t min_duration, mempool_t *listpool,
		const char *argv0)
{
	// get titles
	uint32_t np = bd_get_titles(bd, filter_flags, min_duration);
	for(uint32_t ip = 0; ip < np; ip++)
	{
		BLURAY_TITLE_INFO *ti = bd_get_title_info(bd, ip, 0);
		if(ti == NULL)
			return -1;

		// find insert point
		struct tilist **ptil = ptil_first;
		struct tilist *til = *ptil;
		while(til && til->ti->playlist <= ti->playlist)
		{
			ptil = &til->next;
			til = *ptil;
		}
		// append new title
		til = tilist_create(ptil, bd, ti, NULL, listpool, argv0);
		if(til == NULL)
			return -1;
		til->an[0] = ANGLE_WILDCARD;
	}
	return 0;
}

static int tilist_add_selectors(struct tilist **ptil, BLURAY *bd,
		struct pllist *pl_first, mempool_t *listpool, const char *argv0)
{
	// insert selected playlist
	struct tilist *til = *ptil;
	for(struct pllist *pl = pl_first; pl;)
	{
		// find insert point
		while(til && til->ti->playlist < pl->pl)
		{
			ptil = &til->next;
			til = *ptil;
		}
		// didn't get playlist info already
		BLURAY_TITLE_INFO *ti;
		uint32_t          *clips = NULL;
		uint8_t            ia;
		if(til == NULL || pl->pl != til->ti->playlist)
		{
			ti = bd_get_playlist_info(bd, pl->pl, 0);
			if(ti == NULL)
				return -1;

		tilist_insert_playlist:
			// append new title
			til = tilist_create(ptil, bd, ti, clips, listpool, argv0);
			if(til == NULL)
				return -1;
			ia = 0;
		tilist_add_angle:
			// valid angle
			if(pl->an >= ti->angle_count && pl->an != ANGLE_WILDCARD)
			{
				fprintf(stderr, "%s: Invalid angle %" PRIu8 " for playlist %05"
						PRIu32 ".mpls\n", argv0, pl->an, pl->pl);
				return -1;
			}
			til->an[ia] = pl->an;
			if(ia < sizeof(til->an) / sizeof(til->an[0]) - 1)
				til->an[ia + 1] = ANGLE_WILDCARD;
		}
		else if(til->an[0] != ANGLE_WILDCARD)
		{
			ti    = til->ti;
			clips = til->clips;
			while(til && til->ti == ti)
			{
				for(ia = 1; ia < sizeof(til->an) / sizeof(til->an[0]); ia++)
					if(til->an[ia] == ANGLE_WILDCARD)
						goto tilist_add_angle;
				ptil = &til->next;
				til = *ptil;
			}
			goto tilist_insert_playlist;
		}

		struct pllist *pl2 = pl->next;
		mempool_free(listpool, pl);
		pl = pl2;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ok = 1;

	mempool_t listpool;
	mempool_init(&listpool, 32, MAX(sizeof(struct pllist), sizeof(struct tilist)),
			NULL, NULL, NULL);
	struct pllist *pl_first = NULL;
	struct tilist *ti_first = NULL;
	BLURAY        *bd       = NULL;
	const char   **langs    = NULL;
	const char   **ffargv   = NULL;

	uint32_t min_duration = -1;
	int      filter_flags = TITLES_FILTER_DUP_TITLE;
	int      operation    = 'l';

	const struct option long_options[] = {
			{"time",        required_argument, NULL, 't'},
			{"playlist",    required_argument, NULL, 'p'},
			{"all",         no_argument,       NULL, 'a'},
//			{"multiple",    no_argument,       NULL, 'm'},
			{"info",        no_argument,       NULL, 'i'},
			{"chapters",    no_argument,       NULL, 'c'},
			{"ffmpeg",      required_argument, NULL, 'f'},
			{"remux",       required_argument, NULL, 'x'},
			{"help",        no_argument,       NULL, 'h'},
			{"version",     no_argument,       NULL, 'v'},
			{NULL,          0,                 NULL, 0}};
	char optstring[OPTSTRING_LENGTH(long_options)];
	make_optstring(optstring, long_options);

	// parse arguments
	while(1)
	{
		int c = getopt_long(argc, argv, optstring, long_options, NULL);
		if(c == -1)
			break;
		switch(c)
		{
		case 'h':
			if(printf("Usage: %s [OPTION]... INPUT [OUTPUT]\n"
					"Get " BLURAY_SPELLING " info and extract tracks with ffmpeg.\n"
					"\n"
					"  -t, --time=DURATION        select all titles at least DURATION seconds long\n"
					"  -p, --playlist=PLAYLIST[:ANGLE]\n"
					"                             select playlist PLAYLIST and optionally only angle\n"
					"                             ANGLE\n"
					"  -a, --all                  do not omit duplicate titles\n"
//					"  -m, --multiple             allow selection of multiple titles for extraction\n"
					"  -i, --info                 print more detailed information\n"
					"  -c, --chapters             print chapter xml\n"
					"  -f, --ffmpeg=LANGUAGES     print ffmpeg call to extract streams of given or\n"
					"                             undefined languages\n"
					"  -x, --remux=LANGUAGES      extract streams of given or undefined languages\n"
					"                             with ffmpeg\n"
					"  -h, --help                 display this help and exit\n"
					"  -v, --version              output version information and exit\n",
					argv[0]) < 0)
				goto error_errno;
			return 0;
		case 'v':
			if(printf("%s " VERSION "\n"
					"Copyright (C) 2016 Schnusch\n"
					"License LGPLv3+: GNU LGPL version 3 or later <http://gnu.org/licenses/lgpl.html>.\n"
					"This is free software: you are free to change and redistribute it.\n"
					"There is NO WARRANTY, to the extent permitted by law.\n"
					"\n"
					"Written by Schnusch.\n", argv[0]) < 0)
				goto error_errno;
			return 0;
		case 't':
		{
			// parse time
			uint32_t t;
			if(dec2uint(&t, sizeof(uint32_t), optarg) == -1)
			{
				fprintf(stderr, "%s: Invalid duration %s\n", argv[0], optarg);
				return 1;
			}
			// save smallest time
			if(t < min_duration)
				min_duration = t;
			break;
		}
		case 'p':
		{
			// parse playlist
			uint32_t plnum;
			uint8_t  annum;
			if(parse_playlist_arg(&plnum, &annum, optarg) == -1)
			{
				fprintf(stderr, "%s: Invalid playlist %s\n", argv[0], optarg);
				return 1;
			}

			// find insert point
			struct pllist **ppl = &pl_first;
			struct pllist *pl = *ppl;
			while(pl && (pl->pl < plnum || (pl->pl == plnum && pl->an < annum
					&& annum != ANGLE_WILDCARD && pl->an != ANGLE_WILDCARD)))
			{
				ppl = &(*ppl)->next;
				pl = *ppl;
			}
			// remove obsolete angles
			if(pl && annum == ANGLE_WILDCARD && pl->an != ANGLE_WILDCARD)
			{
				while(pl && pl->pl == plnum)
				{
					*ppl = pl->next;
					mempool_free(&listpool, pl);
					pl = *ppl;
				}
			}
			// avoid duplicates
			if(pl == NULL || pl->pl != plnum
					|| (pl->an != ANGLE_WILDCARD && pl->an != annum))
			{
				// append new playlist
				pl = mempool_alloc(&listpool);
				if(pl == NULL)
					goto error_errno;
				pl->pl   = plnum;
				pl->an   = annum;
				pl->next = *ppl;
				*ppl = pl;
			}
			break;
		}
		case 'a':
			filter_flags = 0;
			break;
		case 'f':
		case 'x':
		{
			// prepare language list
			size_t l = (strcnt(optarg, ',') + 2) * sizeof(char *);
			// count ISO639-2 doubles
			char *c = optarg;
			while(1)
			{
				char *c2 = strchrnul(c, ',');
				if(c2 - c == 3)
				{
					if(prefered_iso6392(c) == c)
					{
						for(const struct iso6392_map *ld = langdoubles; ld->prefered; ld++)
							if(strncmp(ld->prefered, c, 3) == 0)
							{
								l += sizeof(char *);
								break;
							}
					}
					else
						l += sizeof(char *);
				}
				if(*c2 == '\0')
					break;
				c = c2 + 1;
			}
			langs = realloc(langs, l + strlen(optarg) + 1);
			if(langs == NULL)
				goto error_errno;
			c = strcpy((char *)langs + l, optarg);
			const char **lang = langs;
			while(1)
			{
				*lang = prefered_iso6392(c);
				char *c2 = strchrnul(c, ',');
				// ISO639-2 doubles
				if(*lang == c)
				{
					if(c2 - c == 3)
						for(const struct iso6392_map *ld = langdoubles; ld->prefered; ld++)
							if(strncmp(ld->prefered, c, 3) == 0)
							{
								*(++lang) = ld->possible;
								break;
							}
				}
				else
					*(++lang) = c;
				lang++;
				if(*c2 == '\0')
					break;
				*c2 = '\0';
				c = c2 + 1;
			}
			*lang = NULL;
		}
		case 'i':
		case 'c':
			operation = c;
			break;
		}
	}

	const char *src = argv[optind++];
	if(src == NULL)
	{
		fprintf(stderr, "%s: No " BLURAY_SPELLING " specified\n", argv[0]);
		return 0;
	}

	if(min_duration == (uint32_t)-1 && pl_first == NULL)
		min_duration = 0;

	// open bluray
	bd = bd_open(src, NULL);
	if(bd == NULL)
		goto error_libbluray;

	// get BLURAY_TITLE_INFOs by duration
	if(min_duration != (uint32_t)-1)
	{
		errno = 0;
		if(tilist_add_by_duration(&ti_first, bd, filter_flags, min_duration,
				&listpool, argv[0]) == -1)
		{
			if(errno == 0)
				goto error_libbluray;
			else
				goto error_errno;
		}
	}
	// get BLURAY_TITLE_INFOs by playlist selectors
	errno = 0;
	if(tilist_add_selectors(&ti_first, bd, pl_first, &listpool, argv[0]) == -1)
	{
		if(errno == 0)
			goto error_libbluray;
		else
			goto error_errno;
	}

	if(ti_first == NULL)
	{
		fprintf(stderr, "%s: No title selected\n", argv[0]);
		goto error;
	}

	if(operation == 'l' || operation == 'i')
	{
		if(list_titles(ti_first, operation == 'i', true) == -1)
			goto error_errno;
	}
	else
	{
		// single title
		if(ti_first->next != NULL)
		{
			uint32_t nt = 0;
			for(struct tilist *til = ti_first; til; til = til->next)
				nt++;

			char short_op[2];
			const char *long_op = NULL;
			for(const struct option *lo = long_options; lo->name; lo++)
				if(lo->val == operation)
				{
					long_op = lo->name;
					break;
				}
			if(long_op == NULL)
			{
				short_op[0] = operation;
				short_op[1] = '\0';
			}
			fprintf(stderr, "%s: %s%s requires a single title (%" PRIu32
					" selected)\n", argv[0], long_op == NULL ? "-" : "--",
					long_op == NULL ? short_op : long_op, nt);
			goto error;
		}

		BLURAY_TITLE_INFO *ti = ti_first->ti;
		if(operation == 'c')
		{
			if(print_xml_chapters(ti) == -1)
				goto error_errno;
		}
		else
		{
			// destination file
			const char *dst = argv[optind];
			if(dst == NULL)
			{
				fprintf(stderr, "%s: No destination file given\n", argv[0]);
				goto error;
			}

			if(operation == 'f')
			{
				// generate ffmpeg call without chapter pipe
				ffargv = generate_ffargv(ti_first, langs, src, dst, 0);
				if(ffargv == NULL)
					goto error_errno;

				// print ffmpeg call
				for(const char **ffarg = ffargv; *ffarg; ffarg++)
				{
					if(ffarg != ffargv)
						if(fputc(' ', stdout) == EOF)
							goto error_errno;
					if(fputs(shell_escape(*ffarg), stdout) == EOF)
						goto error_errno;
				}
				if(fputs(" << EOF\n", stdout) == EOF)
					goto error_errno;
				if(print_ff_chapters(ti_first->ti) == -1)
					goto error_errno;
				if(fputs("EOF\n", stdout) == EOF)
					goto error_errno;
			}
			else
			{
				int fds[2];
				if(pipe(fds) == -1)
					goto error_errno;

				// generate ffmpeg call with chapter pipe
				ffargv = generate_ffargv(ti_first, langs, src, dst, fds[0]);
				free(langs);
				if(ffargv == NULL)
				{
					close(fds[0]);
					close(fds[1]);
					goto error_errno;
				}

				// start chapter feeding process
				if(start_ffmpeg_chapter_process(bd, ti, fds, argv[0]) == -1)
					goto error;

				bd_free_title_info(ti);
				bd_close(bd);

				// start ffmpeg
				execvp("ffmpeg", ffargv);
				close(fds[0]);
				goto error_errno;
			}
		}
	}

	if(0)
	{
		do
		{
		error_errno:
			perror(argv[0]);
			break;
		error_libbluray:
			fprintf(stderr, "%s: Error in %s\n", argv[0], src);
			break;
		}
		while(0);
	error:
		ok = 0;
	}
	for(struct tilist *til = ti_first; til;)
	{
		if(til->ti != NULL)
			bd_free_title_info(til->ti);
		free(til->clips);
		for(BLURAY_TITLE_INFO *ti = til->ti; til && til->ti == ti; til = til->next);
	}
	mempool_destroy(&listpool);
	free(ffargv);
	free(langs);
	if(bd != NULL)
		bd_close(bd);

	return !ok;
}
