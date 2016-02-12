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

#define VERSION "0.2.2"

#define _GNU_SOURCE
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libbluray/bluray.h>

#include "bd.h"

#define DEC_TO_UINT(name, type) \
		static int dec2 ## name(type *i, const char *s) \
		{ \
			*i = 0; \
			for(const char *c = s; *c; c++) \
				if(__builtin_mul_overflow(*i, 10, i) || *c < '0' || '9' < *c \
						|| __builtin_add_overflow(*i, *c - '0', i)) \
					return -1; \
			return 0; \
		}
DEC_TO_UINT(uint8,  uint8_t)
DEC_TO_UINT(uint32, uint32_t)
DEC_TO_UINT(uint,   unsigned int)
#undef DEC_TO_UINT

char time_buf[13];
const char *ticks2time(uint64_t ticks)
{
	int n = snprintf(time_buf, sizeof(time_buf), "%02" PRIu64 ":%02" PRIu64
			":%02" PRIu64 ".%03" PRIu64,
			ticks / 90000 / 3600, (ticks / 90000 / 60) % 60,
			(ticks / 90000) % 60, (ticks / 90) % 1000);
	if((size_t)n < sizeof(time_buf))
		return time_buf;
	else
		return 0;
}

size_t strcnt(const char *s, int c)
{
	size_t n = 0;
	char *s2 = (char *)s;
	while((s2 = strchr(s2, c)) != NULL)
	{
		s2++;
		n++;
	}
	return n;
}

char *shell = NULL;
/**
 * Escapes a string not solely consisting of \w[-+,./_] for use in an
 * interactive shell.
 * Every call frees the result of the previous call.
 */
const char *shell_escape(const char *src)
{
	size_t n = strlen(src);
	if(strspn(src, "+,-./0123456789:@ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
			"abcdefghijklmnopqrstuvwxyz") == n)
		return src;

	n += strcnt(src, '\'') + 3;
	free(shell);
	shell = malloc(n);
	if(shell != NULL)
	{
		char *dst = shell;
		*dst++ = '\'';
		do
		{
			const char *c = src;
			if(*src == '\'')
			{
				*dst++ = '\\';
				c++;
			}
			c = strchrnul(c, '\'');
			dst = mempcpy(dst, src, c - src);
			src = c;
		}
		while(*src != '\0');
		*dst++ = '\'';
		*dst = '\0';
	}
	return shell;
}

#define FATAL(x) \
		do \
		{ \
			if(x) \
				return -1; \
		} \
		while(0)


/**
 * list available tracks
 */
int list_titles(BLURAY *bd, const char *src, const char *sdur,
		const char *argv0)
{
	uint32_t min_duration = 0;
	if(sdur != NULL && dec2uint32(&min_duration, sdur) == -1)
	{
		fprintf(stderr, "%s: Invalid duration %s\n", argv0, sdur);
		return 1;
	}

	struct bluray_title_pile *pile = get_bluray_title_pile(bd, src,
			min_duration, TITLE_PILE_ALL);
	if(pile == NULL)
		return -1;

	int m = bd_get_main_title(bd);
	for(uint32_t i = 0; i < pile->title_count; i++)
	{
		if(i > 0)
			if(fputc('\n', stdout) == EOF)
			{
			error:
				free_bluray_title_pile(pile);
				return -1;
			}

		struct bluray_title *bt = &pile->titles[i];
		if(printf("%s playlist: %05" PRIu32 ".mlps, angle: %" PRIu8 ", "
				"duration: %s, chapters: %" PRIu32,
				m != -1 && i == (uint32_t)m ? "(*)" : "   ",
				bt->playlist, bt->angle, ticks2time(bt->duration),
				bt->chapter_count) < 0)
			goto error;

		if(bt->clip_count > 0)
		{
			if(printf("\n        %2" PRIu32 " clip%s: ", bt->clip_count,
					bt->clip_count == 1 ? "" : "s") < 0)
				goto error;
			for(uint32_t j = 0; j < bt->clip_count; j++)
			{
				if(j > 0)
					if(fputc(',', stdout) == EOF)
						goto error;
				if(fputs(bt->clips[j].name == NULL ? "(unknown)" :
						bt->clips[j].name, stdout) == EOF)
					goto error;
			}
		}

		if(bt->stream_count > 0)
		{
			unsigned int last = bt->streams[0].type;
			uint16_t j = 0;
			uint16_t k = 0;
			while(bt->stream_count > 0)
			{
				if(j == bt->stream_count || bt->streams[j].type != last)
				{
					if(printf("\n        %2" PRIu16 " %s stream%s %s",
							j - k,
							last == BLURAY_STREAM_VIDEO    ? "video"    :
							last == BLURAY_STREAM_AUDIO    ? "audio"    :
							last == BLURAY_STREAM_SUBTITLE ? "subtitle" :
							"other", j - k == 1 ? ": " : "s:",
							last == BLURAY_STREAM_SUBTITLE ? "" : "   ") < 0)
						goto error;
					last = bt->streams[j].type;
					goto skip_comma;
					while(k < j)
					{
						if(fputc(',', stdout) == EOF)
							goto error;
					skip_comma:
						if(fputs(bt->streams[k].lang, stdout) == EOF)
							goto error;
						k++;
					}
				}
				if(j == bt->stream_count)
					break;
				j++;
			}
		}
		if(fputc('\n', stdout) == EOF)
			goto error;
	}
	return 0;
}


#define STREAM_CHANGE_ERROR(argv0, type, pl, old, new) \
		fprintf(stderr, "%s: Number of " type " tracks in %05" PRIu32 ".mpls" \
				" changed from %" PRIu8 " to %" PRIu8 "\n", argv0, pl, old, new)

int assert_streams(const char *argv0, BLURAY_TITLE_INFO *ti)
{
	// TODO better
	int first = 1;
	uint8_t vids, auds, subs;
	for(uint32_t i = 0; i < ti->clip_count; i++)
	{
		BLURAY_CLIP_INFO *clip = &ti->clips[i];
		if(first)
		{
			vids = clip->video_stream_count;
			auds = clip->audio_stream_count;
			subs = clip->pg_stream_count;
			first = 0;
		}
		else if(clip->video_stream_count != vids)
		{
			STREAM_CHANGE_ERROR(argv0, "video", ti->playlist, vids,
					clip->video_stream_count);
			goto stream_assert;
		}
		else if(clip->audio_stream_count != auds)
		{
			STREAM_CHANGE_ERROR(argv0, "audio", ti->playlist, auds,
					clip->audio_stream_count);
			goto stream_assert;
		}
		else if(clip->pg_stream_count != subs)
		{
			STREAM_CHANGE_ERROR(argv0, "subtitle", ti->playlist, subs,
					clip->pg_stream_count);
			goto stream_assert;
		}
		if(0)
		{
		stream_assert:
			bd_free_title_info(ti);
			return -1;
		}
	}
	return 0;
}


static int print_chapters(BLURAY_TITLE_INFO *ti, const char *head,
		int (*print_chapter)(BLURAY_TITLE_CHAPTER *), const char *tail)
{
	FATAL(fputs(head, stdout) == EOF);
	for(uint32_t i = 0; i < ti->chapter_count; i++)
		FATAL(print_chapter(&ti->chapters[i]) == -1);
	if(tail != NULL)
		FATAL(fputs(tail, stdout) == EOF);
	return 0;
}

static int print_xml_chapter(BLURAY_TITLE_CHAPTER *chap)
{
	FATAL(printf(
			"\t\t<ChapterAtom>\n"
			"\t\t\t<ChapterTimeStart>%s</ChapterTimeStart>\n"
			"\t\t\t<ChapterFlagHidden>0</ChapterFlagHidden>\n"
			"\t\t\t<ChapterFlagEnabled>1</ChapterFlagEnabled>\n"
			"\t\t</ChapterAtom>\n",
			ticks2time(chap->start)) < 0);
	return 0;
}

int print_xml_chapters(BLURAY_TITLE_INFO *ti)
{
	return print_chapters(ti,
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<Chapters>\n"
			"\t<EditionEntry>\n"
			"\t\t<EditionFlagHidden>0</EditionFlagHidden>\n"
			"\t\t<EditionFlagDefault>0</EditionFlagDefault>\n",
			&print_xml_chapter,
			"\t</EditionEntry>\n"
			"</Chapters>\n");
}


static int print_mkvpropedit2(BLURAY_STREAM_INFO *si, uint8_t n, char **tok,
		uint8_t *i, char *mapping)
{
	if(*tok == NULL && mapping != NULL)
		return 0;

	for(uint8_t j = 0; j < n; j++, (*i)++)
	{
		uint8_t itok;
		if(mapping == NULL)
			goto skip;
		while(*tok != NULL)
		{
			FATAL(dec2uint8(&itok, *tok) == -1);
			if(itok >= *i)
				break;
			*tok = strtok(NULL, ",");
		}

		if(*tok != NULL && itok == *i)
		{
		skip:
			if(si[j].lang[0] != '\0')
				FATAL(printf("-e%ctrack:%" PRIu8 "%c-s%clanguage=%s%c",
						0, *i, 0, 0, si[j].lang, 0) < 0);
		}
	}
	return 0;
}

int print_mkvpropedit(BLURAY_TITLE_INFO *ti, char *mapping)
{
	char *tok = strtok(mapping, ",");

	uint8_t i = 1;
	BLURAY_CLIP_INFO *clip = &ti->clips[0];
	FATAL(print_mkvpropedit2(clip->video_streams, clip->video_stream_count,
			&tok, &i, mapping) == -1);
	FATAL(print_mkvpropedit2(clip->audio_streams, clip->audio_stream_count,
			&tok, &i, mapping) == -1);
	FATAL(print_mkvpropedit2(clip->pg_streams, clip->pg_stream_count,
			&tok, &i, mapping) == -1);

	return 0;
}


const char **generate_ffargv(BLURAY *bd, uint32_t playlist, uint8_t angle,
		char **langs, const char *src, const char *dst, int chapters)
{
	// TODO pcm_bluray and maybe custom arguments

	/*
	ffmpeg -playlist %d -angle %d -i bluray:%s [-i /proc/self/fd/%d]
			-c copy [[-map 0:%c:%d] [-metadata:s:%d language=%s]]...
			[-map_chapters 1] %s
	*/
	struct bluray_title_pile *pile = get_bluray_title_pile(bd, src, 0,
			TITLE_PILE_PLAYLIST | TITLE_PILE_ANGLE, playlist, angle);
	if(pile == NULL)
		return NULL;
	struct bluray_title *bt = pile->titles;

	size_t ffargc = 10;
	size_t ffargn = (DECIMAL_BUFFER_LEN(uint32_t) + 1)
			+ (DECIMAL_BUFFER_LEN(uint8_t) + 1) + (7 + strlen(src) + 1);
	if(chapters != -1)
	{
		ffargc += 4;
		ffargn += 14 + DECIMAL_BUFFER_LEN(int) + 1;
	}

	// count mappings
	uint16_t maps  = 0;
	uint16_t metas = 0;
	uint8_t  convs = 0;
	for(uint16_t i = 0; i < bt->stream_count; i++)
		for(char **lang = langs; *lang; lang++)
			if(strcmp(bt->streams[i].lang, *lang) == 0)
			{
				maps++;
				metas++;
				if(bt->streams[i].codec == BLURAY_CODEC_PCM)
				{
					convs += 3 + DECIMAL_BUFFER_LEN(uint16_t);
					ffargc += 4;
				}
				break;
			}
	ffargc += 2 * (maps + metas);
	ffargn += maps * (2 + DECIMAL_BUFFER_LEN(uint16_t) + 1)
			+ metas * (12 + DECIMAL_BUFFER_LEN(uint16_t) + 1 + 12 + 1)
			+ convs;

	const char **ffargv = malloc((ffargc + 1) * sizeof(char *) + ffargn);
	if(ffargv != NULL)
	{
		const char **ffargp1 = ffargv;
		char *ffargp2 = (char *)(ffargv + ffargc + 1);
		*ffargp1++ = "ffmpeg";
		*ffargp1++ = "-playlist";
		ffargp2 += sprintf((char *)(*ffargp1++ = ffargp2),
				"%" PRIu32, playlist) + 1;
		*ffargp1++ = "-angle";
		ffargp2 += sprintf((char *)(*ffargp1++ = ffargp2),
				"%" PRIu8, angle) + 1;
		*ffargp1++ = "-i";
		ffargp2 = stpcpy(stpcpy((char *)(*ffargp1++ = ffargp2),
				"bluray:"), src) + 1;
		if(chapters != -1)
		{
			*ffargp1++ = "-i";
			ffargp2 += sprintf((char *)(*ffargp1++ = ffargp2),
					"/proc/self/fd/%d", chapters) + 1;
		}
		*ffargp1++ = "-c";
		*ffargp1++ = "copy";
		// argvify mappings
		uint16_t j = 0;
		for(uint16_t i = 0; i < bt->stream_count; i++)
			for(char **lang = langs; *lang; lang++)
				if(strcmp(bt->streams[i].lang, *lang) == 0)
				{
					*ffargp1++ = "-map";
					ffargp2 += sprintf((char *)(*ffargp1++ = ffargp2),
							"0:%" PRIu16, bt->streams[i].indx) + 1;
					if(strcmp(bt->streams[i].lang, "und") != 0)
					{
						ffargp2 += sprintf((char *)(*ffargp1++ = ffargp2),
								"-metadata:s:%" PRIu16, j) + 1;
						ffargp2 += sprintf((char *)(*ffargp1++ = ffargp2),
								"language=%s", *lang) + 1;
						if(bt->streams[i].codec == BLURAY_CODEC_PCM)
						{
							ffargp2 += sprintf((char *)(*ffargp1++ = ffargp2),
									"-c:%" PRIu16, j) + 1;
							*ffargp1++ = "flac";
							*ffargp1++ = "-compression_level";
							*ffargp1++ = "12";
						}
					}
					j++;
					break;
				}
		if(chapters != -1)
		{
			*ffargp1++ = "-map_chapters";
			*ffargp1++ = "1";
		}
		*ffargp1++ = dst;
		*ffargp1 = NULL;
	}

	return ffargv;
}

static int print_ff_chapter(BLURAY_TITLE_CHAPTER *chap)
{
	FATAL(printf("[CHAPTER]\n"
			"TIMEBASE=1/90000\n"
			"START=%" PRIu64 "\n"
			"END=%" PRIu64 "\n",
			chap->start, chap->start + chap->duration) < 0);
	return 0;
}

int print_ff_chapters(BLURAY_TITLE_INFO *ti)
{
	return print_chapters(ti, ";FFMETADATA1\n", &print_ff_chapter, NULL);
}

int start_ffmpeg_chapter_process(BLURAY *bd, BLURAY_TITLE_INFO *ti,
		int fds[2], const char *argv0)
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


int main(int argc, char **argv)
{
	int operation = 'l';
	char *argument = NULL;
	uint32_t playlist = 0;
	int playlist_set = 0;
	unsigned int angle = 0;

	// parse options
	while(1)
	{
		static struct option long_options[] = {
				{"list",        optional_argument, NULL, 'l'},
				{"playlist",    required_argument, NULL, 'p'},
				{"angle",       required_argument, NULL, 'a'},
				{"chapters",    no_argument,       NULL, 'c'},
//				{"mkvpropedit", required_argument, NULL, 'm'},
				{"ffmpeg",      required_argument, NULL, 'f'},
				{"remux",       required_argument, NULL, 'x'},
				{"help",        no_argument,       NULL, 'h'},
				{"version",     no_argument,       NULL, 'v'},
				{NULL,          0,                 NULL, 0}};
		int c = getopt_long(argc, argv, "l::p:a:cf:x:hv", long_options, NULL);
		if(c == -1)
			break;
		switch(c)
		{
		case 'h':
			if(printf("Usage: %s [OPTION]... FILE\n"
					"Get BluRay info and extract tracks with ffmpeg.\n"
					"\n"
					"  -l, --list[=DURATION]      list all tracks at least DURATION seconds\n"
					"                             long (default operation)\n"
					"  -p, --playlist=PLAYLIST    use playlist PLAYLIST\n"
					"  -a, --angle=ANGLE          use angle ANGLE\n"
					"  -c, --chapters             print chapter xml and exit (-p required)\n"
//					"  -m, --mkvpropedit=MAPPING  print null-terminated mkvpropedit options to adjust\n"
//					"                             a remuxes metadata (-p required)\n"
					"  -f, --ffmpeg=LANGUAGES     create a ffmpeg call to extract all given languages\n"
					"                             (-p required)\n"
					"  -x, --remux=LANGUAGES      extract all given language's streams and chatpers\n"
					"                             with ffmpeg (-p required)\n"
					"  -h, --help     display this help and exit\n"
					"  -v, --version  output version information and exit\n",
					argv[0]) < 0)
				goto error;
			return 0;
		case 'v':
			if(printf("%s " VERSION "\n"
					"Copyright (C) 2016 Schnusch\n"
					"License LGPLv3+: GNU LGPL version 3 or later <http://gnu.org/licenses/lgpl.html>.\n"
					"This is free software: you are free to change and redistribute it.\n"
					"There is NO WARRANTY, to the extent permitted by law.\n"
					"\n"
					"Written by Schnusch.\n", argv[0]) < 0)
				goto error;
			return 0;
		case 'p':
			if(dec2uint32(&playlist, optarg) == -1)
			{
				fprintf(stderr, "%s: Invalid playlist %s\n", argv[0], optarg);
				return 1;
			}
			playlist_set = 1;
			break;
		case 'a':
			if(dec2uint(&angle, optarg) == -1)
			{
				fprintf(stderr, "%s: Invalid angle %s\n", argv[0], optarg);
				return 1;
			}
			break;
		case 'l':
		case 'f':
//		case 'm':
		case 'x':
			argument = optarg;
		case 'c':
			operation = c;
			break;
		default:
			return 1;
		}
	}
#define CMDARG_ERROR(cond, msg) \
		if(cond) \
		{ \
			fprintf(stderr, "%s: " msg "\n", argv[0]); \
			return 1; \
		}
	const char *src = argv[optind++];
	CMDARG_ERROR(src == NULL, "No bluray specified");
	CMDARG_ERROR((operation == 'f' || operation == 'x') && argv[optind] == NULL,
			"No destination file given");
	CMDARG_ERROR(!playlist_set && operation != 'l', "-p required");
#undef CMDARG_ERROR

	// open playlist
	BLURAY *bd = bd_open(src, NULL);
	if(bd == NULL)
	{
	error:
		perror(argv[0]);
		return 1;
	}

	if(operation == 'l')
	{
		int e = list_titles(bd, src, argument, argv[0]);
		if(e > 0)
		{
		error_silent:
			bd_close(bd);
			return 1;
		}
		else if(e < 0)
		{
		error2:
			bd_close(bd);
			goto error;
		}
	}
	else
	{
		// open playlist
		BLURAY_TITLE_INFO *ti = bd_get_playlist_info(bd, playlist, angle);
		if(ti == NULL)
		{
			fprintf(stderr, "%s: Playlist %" PRIu32 " angle %u not found\n",
					argv[0], playlist, angle);
			goto error_silent;
		}
		if(assert_streams(argv[0], ti) == -1)
		{
		error_silent2:
			bd_free_title_info(ti);
			goto error_silent;
		}

		if(operation == 'c')
		{
			// extract chapters
			if(print_xml_chapters(ti) == -1)
			{
			error3:
				bd_free_title_info(ti);
				goto error2;
			}
		}
		else if(operation == 'm')
		{
			// create mkvpropedit call
			if(print_mkvpropedit(ti, argument) == -1)
				goto error3;
		}
		else
		{
			// prepare language list
			size_t l = (strcnt(argument, ',') + 3) * sizeof(char *);
			char **langs = malloc(l + strlen(argument) + 1);
			if(langs == NULL)
				goto error3;
			char *c = strcpy((char *)langs + l, argument);
			char **lang = langs;
			while(1)
			{
				*lang++ = c;
				char *c2 = strchr(c, ',');
				if(c2 == NULL)
					break;
				*c2 = '\0';
				c = c2 + 1;
			}
			*lang++ = "und";
			*lang = NULL;

			const char *dst = argv[optind];

			const char **ffargv = NULL;
			if(operation == 'f')
			{
				// generate ffmpeg call without chapter pipe
				ffargv = generate_ffargv(bd, playlist, angle, langs, src, dst,
						-1);
				free(langs);
				if(ffargv == NULL)
					goto error3;

				// print ffmpeg call
				for(const char **ffarg = ffargv; *ffarg; ffarg++)
				{
					if(ffarg != ffargv)
						if(fputc(' ', stdout) == EOF)
						{
						error4:
							free(ffargv);
							goto error3;
						}
					if(fputs(shell_escape(*ffarg), stdout) == EOF)
						goto error4;
				}
				free(ffargv);
				if(fputc('\n', stdout) == EOF)
					goto error3;
			}
			else
			{
				int fds[2];
				if(pipe(fds) == -1)
				{
					free(langs);
					goto error3;
				}

				// generate ffmpeg call with chapter pipe
				ffargv = generate_ffargv(bd, playlist, angle, langs, src, dst,
						fds[0]);
				free(langs);
				if(ffargv == NULL)
				{
					close(fds[0]);
					close(fds[1]);
					goto error3;
				}

				// start chapter feeding process
				if(start_ffmpeg_chapter_process(bd, ti, fds, argv[0]) == -1)
				{
					free(ffargv);
					goto error_silent2;
				}

				bd_free_title_info(ti);
				bd_close(bd);

				// start ffmpeg
				execvp("ffmpeg", ffargv);
				close(fds[0]);
				goto error4;
			}
		}
		bd_free_title_info(ti);
	}
	bd_close(bd);

	return 0;
}
