#define _XOPEN_SOURCE 600
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file/filesystem.h"
#include "libbluray/bdnav/mpls_parse.h"
MPLS_PL *_mpls_parse(BD_FILE_H *fp);

#include "mpls.h"
#include "util.h"

struct memfile {
	uint8_t *data;
	int64_t length;
	int64_t offset;
};

static int64_t memtell(BD_FILE_H *fp)
{
	return ((struct memfile *)fp->internal)->offset;
}

static int64_t memseek(BD_FILE_H *fp, int64_t offset, int32_t whence)
{
	struct memfile *mf = fp->internal;
	if(whence == SEEK_SET && offset >= 0)
		mf->offset = offset;
	else if(whence == SEEK_CUR && mf->offset + offset >= 0)
		mf->offset = mf->offset + offset;
	else if(whence == SEEK_END && mf->length + offset >= 0)
		mf->offset = mf->length + offset;
	else
		return -1;
	if(mf->offset > mf->length)
		mf->offset = mf->length;
	return fp->tell(fp);
}

static int64_t memread(BD_FILE_H *fp, uint8_t *buf, int64_t n)
{
	if(n <= 0)
		return 0;
	struct memfile *mf = fp->internal;
	if(n > mf->length - mf->offset)
		n = mf->length - mf->offset;
	memcpy(buf, mf->data + mf->offset, (size_t)n);
	mf->offset += n;
	return (int64_t)n;
}

static BD_FILE_H *memopen(uint8_t *data, size_t n)
{
	BD_FILE_H *bdfp = malloc(sizeof(BD_FILE_H) + sizeof(struct memfile));
	if(bdfp == NULL)
		return NULL;
	struct memfile *mf = (struct memfile *)(bdfp + 1);
	bdfp->internal = mf;
	bdfp->close    = NULL;
	bdfp->seek     = &memseek;
	bdfp->tell     = &memtell;
	bdfp->eof      = NULL;
	bdfp->read     = &memread;
	bdfp->write    = NULL;
	mf->data   = data;
	mf->length = n;
	mf->offset = 0;
	return bdfp;
}

int mpls_memparse(uint32_t *clips, uint8_t *data, size_t n, uint8_t na,
		uint32_t nc)
{
	int err = 0;
	BD_FILE_H *bdfp = NULL;
	MPLS_PL   *pl   = NULL;

	bdfp = memopen(data, n);
	if(bdfp == NULL)
		goto error;

	pl = _mpls_parse(bdfp);
	if(pl == NULL)
		goto error;

	if(pl->list_count != nc || pl->list_count == 0
			|| pl->play_item[0].angle_count != na)
		goto error;

	for(uint32_t ic = 0; ic < pl->list_count; ic++)
	{
		MPLS_PI *pi = &pl->play_item[ic];
		for(uint8_t ia = 0; ia < pi->angle_count; ia++)
		{
			char *id = pi->clip[ia].clip_id;
			if(id[5] != '\0' || dec2uint(&clips[ia * nc + ic], sizeof(uint32_t),
					id) == -1)
				goto error;
		}
	}

	if(0)
	{
	error:
		err = -1;
	}
	mpls_free(pl);
	free(bdfp);

	return err;
}
