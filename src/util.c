/*
Copyright (C) 2016, 2018 Schnusch

This file is part of bdinfo.

bdinfo is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

bdinfo is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License along
with bdinfo.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

const char *iter_comma_list(const char **next, int c)
{
	while(**next == c)
		(*next)++;
	if(!**next)
		return NULL;
	const char *start = *next;
	*next = strchrnul(*next, c);
	return start;
}

void *array_reserve(void *base, size_t nmemb, size_t nmore, size_t size)
{
	static const size_t chunksize = 16;
	size_t last = nmemb - 1;
	if((last + nmore) / chunksize == last / chunksize && nmemb > 0 && base)
		return base;
	nmemb += nmore + (chunksize - 1);
	nmemb -= nmemb % chunksize;
	return realloc(base, nmemb * size);
}

size_t bisect_left(const void *base_, const void *key_, size_t nmemb,
		size_t size, compar_fn compar)
{
	const char *base = base_;
	const char *key  = key_;
	size_t lo = 0;
	size_t hi = nmemb;
	while(lo < hi)
	{
		size_t mid = (lo + hi) / 2;
		if(compar(base + mid * size, key) < 0)
			lo = mid + 1;
		else
			hi = mid;
	}
	return lo;
}

int bisect_contains(const void *base, const void *key, size_t nmemb,
		size_t size, compar_fn compar)
{
	size_t i = bisect_left(base, key, nmemb, size, compar);
	return i < nmemb && compar((char *)base + i * size, key) == 0;
}

char *ticks2time(char timebuf[22], uint64_t ticks)
{
	uint64_t secs = ticks / 90000;
	int n = snprintf(timebuf, 22, "%02"PRIu64":%02"PRIu64":%02"PRIu64".%03"PRIu64,
			secs / 3600, (secs / 60) % 60, secs % 60, (ticks / 90) % 1000);
	if(0 <= n && n < 22)
		return timebuf;
	else
		return NULL;
}

size_t strcnt(const char *s, int c)
{
	size_t n = 0;
	while((s = strchr(s, c)))
		s++, n++;
	return n;
}

const char *shell_escape(const char *src)
{
	static char *shell = NULL;
	free(shell);
	shell = NULL;

	size_t n = strlen(src);
	if(strspn(src, "+,-./0123456789:@ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
			"abcdefghijklmnopqrstuvwxyz") == n)
		return src;

	n += strcnt(src, '\'') * 3 + 3;
	shell = malloc(n);
	if(shell)
	{
		char *dst = shell;
		*dst++ = '\'';
		do
		{
			const char *c = src;
			if(*src == '\'')
			{
				memcpy(dst, "'\\'", 3);
				dst += 3;
				c   += 3;
			}
			c = strchrnul(c, '\'');
			dst = mempcpy(dst, src, c - src);
			src = c;
		}
		while(*src != '\0');
		*dst++ = '\'';
		*dst   = '\0';
	}
	return shell;
}
