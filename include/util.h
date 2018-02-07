#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "location.h"

/* TODO: This is really, really dumb. I think. */
#define EPSILON 0.001
#define fcmp(a,b) (fabs(a - b) <= EPSILON * fabs(a))

/* debug out */
#define DOUT(...)                                     \
	do {                                          \
		fprintf(stderr, "oak: "__FILE__":%d: ", __LINE__); \
		fprintf(stderr, __VA_ARGS__); \
		fputc('\n', stderr);                  \
	} while (0)

static inline bool
is_whitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static inline bool
is_identifier_start(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'));
}

static inline void
append_char(char *s, char c)
{
	char *a = strchr(s, 0);
	*a = c;
	a[1] = 0;
}

static inline bool
is_legal_in_identifier(char c)
{
	return ((c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| c == '_' || c == '-'
		|| (c >= '0' && c <= '9'));
}

static inline bool
is_hex_digit(char c)
{
	return ((c >= 'a' && c <= 'f')
		|| (c >= 'A' && c <= 'F'))
		|| (c >= '0' && c <= '9');
}

static inline bool
is_oct_digit(char c)
{
	return (c >= '0' && c <= '7');
}

static inline bool
is_dec_digit(char c)
{
	return (c >= '0' && c <= '9');
}

static inline int
uc(int c)
{
	return (c >= 'a' && c <= 'z') ? c + ('A' - 'a') : c;
}

static inline int
lc(int c)
{
	return (c >= 'A' && c <= 'Z') ? c - ('A' - 'a') : c;
}

size_t line_number    (struct location loc);
size_t column_number  (struct location loc);
size_t line_len       (struct location loc);
size_t index_in_line  (struct location loc);
char *get_line        (struct location loc);
void chop_extension(char *str);
char *add_extension(char *str);
char *strclone     (const char *str);

void print_escaped_string(FILE *f, char *str, size_t len);
char *substr(const char *str, size_t x, size_t y);

char *smart_cat      (char *lhs, char *rhs);
char *new_cat        (char *lhs, char *rhs);

void remove_char     (char *lhs, size_t c);

uint64_t hash(char *d, size_t len);
char *strsort(const char *s);

void *oak_malloc(size_t size);
void *oak_realloc(void *mem, size_t size);
char *load_file(const char *path);

#endif
