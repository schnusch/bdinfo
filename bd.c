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

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef NO_LIBAVFORMAT
	#include <libavformat/avformat.h>
	#include <libavformat/avio.h>
#endif

#include "bd.h"
#include "mempool.h"

#ifndef NO_CLIP_NAMES
struct clip_map {
	ino_t ino;
	char *name;
};

static struct clip_map *create_clip_map(const char *src)
{
	size_t n = strlen(src);
	char *clip_dir = malloc(n + 14 + sizeof(((struct dirent *)NULL)->d_name)); // hopefully no pointer
	if(clip_dir == NULL)
		return NULL;
	char *e = mempcpy(clip_dir, src, n);
	e = stpcpy(e, "/BDMV/STREAM/");

	DIR *dir = opendir(clip_dir);
	if(dir == NULL)
	{
	error:
		free(clip_dir);
		return NULL;
	}

	// calculate memory requirements
	size_t nclips = 1;
	size_t nnames = 0;
	struct dirent *ent = NULL;
	errno = 0;
	while((ent = readdir(dir)) != NULL)
	{
		if(ent->d_name[0] == '.' && (ent->d_name[1] == '\0'
				|| (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
			continue;

		nclips += 1;
		nnames += strlen(ent->d_name) + 1;
	}
	if(errno)
	{
	error2:
		closedir(dir);
		goto error;
	}
	rewinddir(dir);

	// allocate clip_map
	struct clip_map *clips = malloc(nclips * sizeof(struct clip_map) + nnames);
	if(clips == NULL)
		goto error2;

	// populate clip_map
	char *names = (char *)(clips + nclips);
	struct clip_map *clip = clips;
	struct stat stt;
	errno = 0;
	while((ent = readdir(dir)) != NULL)
	{
		if(ent->d_name[0] == '.' && (ent->d_name[1] == '\0'
				|| (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
			continue;

		strcpy(e, ent->d_name);
		if(stat(clip_dir, &stt) == -1)
		{
		error3:
			free(clips);
			goto error2;
		}
		names = stpcpy(clip->name = names, ent->d_name) + 1;
		clip++->ino = stt.st_ino;
	}
	if(errno != 0)
		goto error3;
	closedir(dir);
	free(clip_dir);
	clip->name = NULL;

	return clips;
}

/**
 * Seek into every clip and check open fds to find the files corresponding to
 * the clips.
 * NOT WORKING
 */
static int do_da_fd_trickery(struct bluray_clip *bc, BLURAY *bd,
		BLURAY_CLIP_INFO *ci, struct clip_map *clips)
{
	if(clips == NULL)
	{
		bc->name = NULL;
		return 0;
	}

	char *fd_dir = malloc(14 + DECIMAL_BUFFER_LEN(int) + 1);
	if(fd_dir == NULL)
		return -1;
	char *e = stpcpy(fd_dir, "/proc/self/fd/");

	bc->name = NULL;

	bd_seek_time(bd, ci->start_time);

	DIR *dir = opendir(fd_dir);
	if(dir == NULL)
	{
	error:
		free(fd_dir);
		return -1;
	}
	struct dirent *ent;
	struct stat stt;
	errno = 0;
	while((ent = readdir(dir)) != NULL)
	{
		if(ent->d_name[0] == '.' && (ent->d_name[1] == '\0'
				|| (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
			continue;
		if((ent->d_name[0] == '0' || ent->d_name[0] == '1'
				|| ent->d_name[0] == '2') && ent->d_name[0] == '\0')
			continue;

		strcpy(e, ent->d_name);
		if(stat(fd_dir, &stt) == -1)
		{
		error2:
			closedir(dir);
			goto error;
		}

		for(struct clip_map *clip = clips; clip->name; clip++)
			if(clip->ino == stt.st_ino)
			{
				bc->name = clip->name;
				goto name_set;
			}
	}
name_set:
	if(errno != 0)
		goto error2;
	closedir(dir);
	free(fd_dir);
	return 0;
}
#endif

struct bluray_clip2 {
	struct bluray_clip base;
	struct bluray_clip *next;
};

struct bluray_stream2 {
	struct bluray_stream base;
	struct bluray_stream *next;
};

struct bluray_title2 {
	struct bluray_title base;
	struct bluray_title *next;
};

#define NEXT(t, x) ((struct bluray_##t##2 *)(x))->next

#ifndef NO_LIBAVFORMAT
/**
 * avoid warning
 */
static int bd_read2(void *bd, uint8_t *buf, int n)
{
	return bd_read(bd, buf, n);
}

	#define AVIOBUFSIZE 4096

static struct bluray_stream *get_avformat_streams(BLURAY *bd, mempool_t *bspool,
		uint16_t *nbss)
{
	uint8_t         *aviobuf  = NULL;
	AVIOContext     *avioctx  = NULL;
	AVFormatContext *avfmtctx = NULL;

	struct bluray_stream *bs_first = NULL;
	struct bluray_stream **pbs = &bs_first;

	aviobuf = av_malloc(AVIOBUFSIZE);
	if(aviobuf == NULL)
		return NULL;

	avioctx = avio_alloc_context(aviobuf, AVIOBUFSIZE, 0, bd, &bd_read2, NULL,
			NULL);
	if(avioctx == NULL)
	{
		av_free(aviobuf);
		goto error;
	}
	bd_seek(bd, 0);

	avfmtctx = avformat_alloc_context();
	if(avfmtctx == NULL)
		goto error;
	avfmtctx->pb = avioctx;

	if(avformat_open_input(&avfmtctx, NULL, NULL, NULL) < 0)
		goto error;

	if(avformat_find_stream_info(avfmtctx, NULL) < 0)
		goto error;

	*nbss = avfmtctx->nb_streams;
	for(uint16_t s = 0; s < avfmtctx->nb_streams; s++)
	{
		AVStream       *avs = avfmtctx->streams[s];
		AVCodecContext *avc = avs->codec;

		struct bluray_stream *bs = *pbs = mempool_alloc(bspool);
		if(*pbs == NULL)
			goto error;
		pbs = &NEXT(stream, *pbs);

		bs->indx  = s;
		bs->id    = avs->id;
		bs->codec = BLURAY_CODEC_OTHER;
		if(avc->codec_type == AVMEDIA_TYPE_VIDEO)
			bs->type = BLURAY_STREAM_VIDEO;
		else if(avc->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			bs->type = BLURAY_STREAM_AUDIO;
			if(avc->codec_id == AV_CODEC_ID_PCM_BLURAY)
				bs->codec = BLURAY_CODEC_PCM;
		}
		else if(avc->codec_type == AVMEDIA_TYPE_SUBTITLE)
			bs->type = BLURAY_STREAM_SUBTITLE;
		else
			bs->type = BLURAY_STREAM_OTHER;
		memcpy(bs->lang, "und", 4);
	}
	*pbs = NULL;

	if(0)
	{
	error:
		bs_first = NULL;
	}
	avformat_close_input(&avfmtctx);
	avformat_free_context(avfmtctx);
	if(avioctx != NULL)
		av_free(avioctx->buffer);
	av_free(avioctx);

	return bs_first;
}
#endif

/**
 * Add languages to struct bluray_stream list \pbs_first.
 */
static void add_metadata(struct bluray_stream *bs_first,
		BLURAY_STREAM_INFO *sis, uint8_t n)
{
	for(struct bluray_stream *bs = bs_first; bs; bs = NEXT(stream, bs))
		for(uint8_t s = 0; s < n; s++)
		{
			BLURAY_STREAM_INFO *si = &sis[s];
			if(si->pid == bs->id)
			{
				if(si->lang[0] != '\0')
					memcpy(bs->lang, si->lang, 4);
				break;
			}
		}
}

static int sort_streams(const void *a_, const void *b_)
{
	const struct bluray_stream *a = a_;
	const struct bluray_stream *b = b_;
	int ret = a->type - b->type;
	if(ret != 0)
		return ret;
	else
		return a->indx - b->indx;
}

struct bluray_title_pile *get_bluray_title_pile(BLURAY *bd, const char *src,
		uint32_t min_duration, int flags, ...)
{
	struct clip_map *clips = NULL;
#ifndef NO_CLIP_NAMES
	clips = create_clip_map(src);
#endif

	mempool_t btpool;
	mempool_t bcpool;
	mempool_t bspool;
	mempool_init(&btpool, 32, sizeof(struct bluray_title2),  NULL, NULL, NULL);
	mempool_init(&bcpool, 32, sizeof(struct bluray_clip2),   NULL, NULL, NULL);
	mempool_init(&bspool, 32, sizeof(struct bluray_stream2), NULL, NULL, NULL);
	struct bluray_title *bt_first;
	struct bluray_title **pbt = &bt_first;

#ifndef NO_LIBAVFORMAT
	static int ffregistered = 0;
	if(!ffregistered)
	{
		av_register_all();
		ffregistered = 1;
	}
#endif

	size_t nbts = 0;
	size_t nbcs = 0;
	size_t nbss = 0;
	size_t nchs = 0;

	// the args
	va_list al;
	va_start(al, flags);
	uint32_t playlist = 0;
	uint8_t  angle    = 0;
	uint32_t ntitles  = 1;
	if(flags & TITLE_PILE_PLAYLIST)
	{
		playlist = va_arg(al, uint32_t);
		if(flags & TITLE_PILE_ANGLE)
			angle = va_arg(al, int);
	}
	else
		ntitles = bd_get_titles(bd, TITLES_ALL, min_duration);
	va_end(al);

	for(uint32_t t = 0; t < ntitles; t++)
	{
		uint8_t angles = angle + 1;
		for(uint8_t a = angle; a < angles; a++)
		{
			BLURAY_TITLE_INFO *ti;
			if(flags & TITLE_PILE_PLAYLIST)
				ti = bd_get_playlist_info(bd, playlist, a);
			else
				ti = bd_get_title_info(bd, t, a);
			if(ti == NULL)
			{
			error:
				*pbt = NULL;
				for(struct bluray_title *bt = bt_first; bt; bt = NEXT(title, bt))
					free(bt->chapters);
				free(clips);
				mempool_destroy(&btpool);
				mempool_destroy(&bspool);
				mempool_destroy(&bcpool);
				return NULL;
			}
			angles = ti->angle_count;

			struct bluray_title *bt = *pbt = mempool_alloc(&btpool);
			if(*pbt == NULL)
			{
				free(bt);
				goto error;
			}
			pbt = &NEXT(title, *pbt);
			bt->chapters = NULL; // for emergency free

			if(!bd_select_playlist(bd, ti->playlist) || !bd_select_angle(bd, a))
				goto error;

			// clips
			nbcs += (bt->clip_count = ti->clip_count);
			struct bluray_clip **pbc = &bt->clips;
			for(uint32_t c = 0; c < ti->clip_count; c++)
			{
				struct bluray_clip *bc = *pbc = mempool_alloc(&bcpool);
				if(*pbc == NULL)
					goto error;
				pbc = &NEXT(clip, *pbc);

#ifdef NO_CLIP_NAMES
				bc->name = NULL;
#else
				if(do_da_fd_trickery(bc, bd, ti->clips, clips) == -1)
					goto error;
#endif
			}
			*pbc = NULL;

#ifdef NO_LIBAVFORMAT
	#error Well, currently you need libavformat
#else
			// libavformat's streams
			bt->streams = get_avformat_streams(bd, &bspool, &bt->stream_count);
			if(bt->streams == NULL)
				goto error;
			nbss += bt->stream_count;
#endif

			// refine with metadata
			BLURAY_CLIP_INFO *c = ti->clips;
			add_metadata(bt->streams, c->video_streams, c->video_stream_count);
			add_metadata(bt->streams, c->audio_streams, c->audio_stream_count);
			add_metadata(bt->streams, c->pg_streams,    c->pg_stream_count);

			// chapters
			bt->chapters = malloc(ti->chapter_count * sizeof(uint64_t));
			if(bt->chapters == NULL)
				goto error;
			for(uint32_t c = 0; c < ti->chapter_count; c++)
				bt->chapters[c] = ti->chapters[c].start;

			// title
			bt->playlist = ti->playlist;
			bt->angle    = a;
			bt->duration = ti->duration;
			nchs += bt->chapter_count = ti->chapter_count;

			bd_free_title_info(ti);
			nbts++;

			if(flags & TITLE_PILE_ANGLE)
				break;
		}
		if(flags & TITLE_PILE_PLAYLIST)
			break;
	}
	*pbt = NULL;

	// TODO find identical clips

	struct bluray_title_pile *pile = malloc(sizeof(struct bluray_title_pile)
			+ nbts * sizeof(struct bluray_title)
			+ nbcs * sizeof(struct bluray_clip)
			+ nbss * sizeof(struct bluray_stream) + nchs * sizeof(uint64_t));
	if(pile == NULL)
		goto error;
	struct bluray_title  *bt = pile->titles;
	struct bluray_clip   *bc = (struct bluray_clip   *)(bt + nbts);
	struct bluray_stream *bs = (struct bluray_stream *)(bc + nbcs);
	uint64_t *ch = (uint64_t *)(bs + nbss);

	// populate memory
	pile->title_count = 0;
	for(struct bluray_title *obt = bt_first; obt; obt = NEXT(title, obt))
	{
		memcpy(bt, obt, sizeof(struct bluray_title));
		// add clips
		bt->clips = bc;
		for(struct bluray_clip *obc = obt->clips; obc; obc = NEXT(clip, obc))
		{
			memcpy(bc, obc, sizeof(struct bluray_clip));
			bc++;
		}
		// add streams
		bt->streams = bs;
		for(struct bluray_stream *obs = obt->streams; obs; obs = NEXT(stream, obs))
		{
			memcpy(bs, obs, sizeof(struct bluray_stream));
			bs++;
		}
		qsort(bt->streams, bt->stream_count, sizeof(struct bluray_stream),
				&sort_streams);
		// add chapters
		ch = mempcpy(bt->chapters = ch, obt->chapters,
				obt->chapter_count * sizeof(uint64_t));
		free(obt->chapters);

		bt++;
	}
	pile->clips = clips;
	pile->title_count = nbts;

	mempool_destroy(&btpool);
	mempool_destroy(&bspool);
	mempool_destroy(&bcpool);

	return pile;
}

void free_bluray_title_pile(struct bluray_title_pile *pile)
{
	free(pile->clips);
	free(pile);
}
