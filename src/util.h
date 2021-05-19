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

/**
 * Iterate over words in a string. Words are separated by the character *c*.
 * *\*next* should be initialized to the begining of the string.
 *
 * Returns a pointer to the word or NULL when finished.
 */
const char *iter_comma_list(const char **next, int c);

/**
 * A lazy alloc.
 *
 * Reallocate *base* in steps of 16 blocks of *nmemb*. *base* holds enough space
 * for `n * 16 * nmemb` bytes and will only be resized if there are not
 * `nmore * nmemb` bytes left anymore.
 */
void *array_reserve(void *base, size_t nmemb, size_t nmore, size_t size);

/**
 * Binary search in *base_*. Returns the index of the element in the array or
 * where it should be inserted.
 */
size_t bisect_left(const void *base_, const void *key_, size_t nmemb,
		size_t size, compar_fn compar);

/**
 * Use binary search to test for presence in a sorted array.
 */
int bisect_contains(const void *base, const void *key, size_t nmemb,
		size_t size, compar_fn compar);

/**
 * Create a string of the format HH:MM:SS.mmm representing the timestamp given
 * in *ticks*.
 */
char *ticks2time(char buf[22], uint64_t ticks);

/**
 * Count occurences of *c* in *s*.
 */
size_t strcnt(const char *s, int c);

/**
 * Escapes a string not solely consisting of [\w-+,./_] for use in an
 * interactive shell.
 * Every call frees the result of the previous call.
 */
const char *shell_escape(const char *src);

#endif
