#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>

#include "location.h"

size_t line_number(struct Location loc);
size_t column_number(struct Location loc);
char* get_line(struct Location loc);

void* oak_malloc(size_t size);
char* load_file(const char* path);

#endif /* UTIL_H */
