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

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <getopt.h>
#include <limits.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define LOG10TO2               3.3219280242919921875f
#define DECIMAL_BUFFER_LEN(t)  ((size_t)(sizeof(t) * CHAR_BIT / LOG10TO2) + 2)

#define OPTSTRING_LENGTH(x)  (sizeof(x) / sizeof((x)[0]) * 3 + 1)

int dec2uint(void *num, size_t n, const char *s);

const char *ticks2time(uint64_t ticks);

size_t strcnt(const char *s, int c);

/**
 * Escapes a string not solely consisting of [\w-+,./_] for use in an
 * interactive shell.
 * Every call frees the result of the previous call.
 */
const char *shell_escape(const char *src);

/**
 * Generate getopt_long's optstring. \poptstring should be
 * sizeof(\plong_opts) / sizeof(\plong_opts[0]) * 3 + 1 bytes long.
 */
void make_optstring(char *optstring, const struct option *long_opts);

#endif
