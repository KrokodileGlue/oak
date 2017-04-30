#ifndef ERROR_H
#define ERROR_H

#include "location.h"

enum ErrorLevel {
	ERR_NOTE = 0, ERR_WARN = 1, ERR_FATAL = 2
};

void push_error(struct Location loc, enum ErrorLevel level, char* fmt, ...);
void write_errors(FILE* fp);

#endif
