#ifndef ERROR_H
#define ERROR_H

#include "location.h"
#include <stdbool.h>

struct Error {
	struct Location loc;
	enum ErrorLevel {
		ERR_NOTE, ERR_WARNING, ERR_FATAL
	} sev;
	char* msg;
};

struct ErrorState {
	struct Error* err;
	size_t num;
	bool pending, fatal;
};

void error_push (struct ErrorState* es, struct Error err, char* fmt, ...);
void error_write(struct ErrorState* es, FILE* fp);
void error_clear(struct ErrorState* es);

#endif
