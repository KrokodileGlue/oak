#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>

size_t line_number(const char* str, size_t location);
size_t column_number(const char* str, size_t location);
char* get_line(const char* str, size_t location);

void* oak_malloc(size_t size);
char* load_file(const char* path);

#endif /* UTIL_H */
