#ifndef ERROR_H
#define ERROR_H

#include "location.h"
#include <stdbool.h>

#define ERR_MAX_MESSAGE_LEN 1024 /* probably enough */

struct Error {
	struct Location loc;
	enum ErrorLevel {
		ERR_NOTE, ERR_WARNING, ERR_FATAL
	} sev;
	char *msg;
};

struct ErrorState {
	struct Error *err;
	size_t num;
	bool pending, fatal;
};

struct ErrorState *error_new();
void error_push (struct ErrorState *es, struct Location loc, enum ErrorLevel sev, char *fmt, ...);
void error_write(struct ErrorState *es, FILE *fp);
void error_clear(struct ErrorState *es);

#endif
