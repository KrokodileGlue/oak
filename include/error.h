#ifndef ERROR_H
#define ERROR_H

#include "location.h"
#include <stdbool.h>

enum ErrorLevel {
	ERR_NOTE = 0, ERR_WARN = 1, ERR_FATAL = 2
};

#define ERR_EOL (size_t)(-1)

bool encountered_error();
void check_errors(FILE* fp);
void push_error(struct Location loc, enum ErrorLevel level, size_t len, char* fmt, ...);
void write_errors(FILE* fp);

#endif
