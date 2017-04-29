#ifndef ERROR_H
#define ERROR_H

#include "location.h"

enum ErrorLevel {
	ERR_NOTE, ERR_WARN, ERR_FATAL
};

void push_error(struct Location loc, enum ErrorLevel level, char* message);
void write_errors(FILE* fp);

#endif
