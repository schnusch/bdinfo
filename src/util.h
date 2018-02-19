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

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <errno.h>
#include <unistd.h>

typedef int (*compar_fn)(const void *, const void *);

const char *iter_comma_list(const char **next, int c);

void *array_reserve(void *base, size_t nmemb, size_t nmore, size_t size);

size_t bisect_left(const void *base_, const void *key_, size_t nmemb,
		size_t size, compar_fn compar);

int bisect_contains(const void *base, const void *key, size_t nmemb,
		size_t size, compar_fn compar);

char *ticks2time(char buf[22], uint64_t ticks);

size_t strcnt(const char *s, int c);

/**
 * Escapes a string not solely consisting of [\w-+,./_] for use in an
 * interactive shell.
 * Every call frees the result of the previous call.
 */
const char *shell_escape(const char *src);

#endif
