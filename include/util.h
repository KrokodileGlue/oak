#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "location.h"

size_t line_number(struct Location loc);
size_t column_number(struct Location loc);
size_t index_in_line(struct Location loc);
char* get_line(struct Location loc);

void* oak_malloc(size_t size);
char* load_file(const char* path);

static inline bool is_whitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static inline bool is_identifier_start(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_');
}

static inline bool is_legal_in_identifier(char c)
{
	return ((c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| c == '_'
		|| (c >= '0' && c <= '9'));
}

static inline bool is_hex_digit(char c)
{
	return ((c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z'))
		|| (c >= '0' && c <= '9');
}

static inline bool is_oct_digit(char c)
{
	return (c >= '0' && c <= '7');
}

#endif /* UTIL_H */
