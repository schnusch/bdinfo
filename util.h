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
