/* Copyright (C) 2016 Schnusch

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
   along with bdinfo.  If not, see <http://www.gnu.org/licenses/>. */

#define _GNU_SOURCE
#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#if BYTE_ORDER == LITTLE_ENDIAN
	#define LEAST_N_BYTES(x, n) (x)
#elif BYTE_ORDER == BIG_ENDIAN
	#define LEAST_N_BYTES(x, n) ((char *)(x) + sizeof(*(x)))
#else
	#error Endianess not supported
#endif

int dec2uint(void *num, size_t n, const char *s)
{
	if(n > sizeof(unsigned long long))
		return -1;
	unsigned long long i = 0;
	unsigned long long mask = ~(((unsigned long long)1 << (n * CHAR_BIT)) - 1);
	for(const char *c = s; *c; c++)
	{
#if __GNUC__ >= 5
		if(*c < '0' || '9' < *c || __builtin_mul_overflow(i, 10, &i)
				|| __builtin_add_overflow(i, *c - '0', &i))
			return -1;
#else
		if(*c < '0' || '9' < *c)
			return -1;
		unsigned long long j = i;
		i *= 10;
		if(i < j)
			return -1;
		j = i;
		i += *c - '0';
		if(i < j)
			return -1;
#endif
		if(i & mask)
			return -1;
	}
	memcpy(num, LEAST_N_BYTES(&i, n), n);
	return 0;
}

static char time_buf[13];
const char *ticks2time(uint64_t ticks)
{
	int n = snprintf(time_buf, sizeof(time_buf), "%02" PRIu64 ":%02" PRIu64
			":%02" PRIu64 ".%03" PRIu64,
			ticks / 90000 / 3600, (ticks / 90000 / 60) % 60,
			(ticks / 90000) % 60, (ticks / 90) % 1000);
	if(0 <= n && (size_t)n < sizeof(time_buf))
		return time_buf;
	else
		return NULL;
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

/**
 * Escapes a string not solely consisting of \w[-+,./_] for use in an
 * interactive shell.
 * Every call frees the result of the previous call.
 */
const char *shell_escape(const char *src)
{
	static char *shell = NULL;
	size_t n = strlen(src);
	if(strspn(src, "+,-./0123456789:@ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
			"abcdefghijklmnopqrstuvwxyz") == n)
		return src;

	n += strcnt(src, '\'') * 3 + 3;
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
		*dst = '\0';
	}
	return shell;
}

/**
 * Generate getopt_long's optstring. \poptstring should be
 * sizeof(\plong_opts) / sizeof(\plong_opts[0]) * 3 + 1 bytes long.
 */
void make_optstring(char *optstring, const struct option *long_opts)
{
	size_t n = 1;
	for(const struct option *lo = long_opts; lo->name; lo++)
	{
		switch(lo->has_arg)
		{
		case optional_argument:
			n++;
		case required_argument:
			n++;
		case no_argument:
			n++;
			break;
		default:
			abort();
		}
	}
	char *s = optstring;
	for(const struct option *lo = long_opts; lo->name; lo++)
	{
		*s++ = (char)lo->val;
		switch(lo->has_arg)
		{
		case optional_argument:
			*s++ = ':';
		case required_argument:
			*s++ = ':';
		}
	}
	*s = '\0';
}
