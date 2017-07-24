#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "location.h"

/* debug out */
#define DOUT(...)                             \
	fprintf(stderr, "oak: " __VA_ARGS__); \
	fputc('\n', stderr)

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

size_t line_number    (struct location loc);
size_t column_number  (struct location loc);
size_t line_len       (struct location loc);
size_t index_in_line  (struct location loc);
void   chop_extension (char *str);
char  *get_line       (struct location loc);
char  *strclone       (char *str);
char  *add_extension  (char *str);
void   print_escaped_string(FILE *f, char *str, size_t len);

void  *oak_malloc     (size_t size);
void  *oak_realloc    (void *mem, size_t size);
char  *load_file      (const char *path);

#endif
