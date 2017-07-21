#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include <stdio.h>
#include "location.h"

#define ERR_MAX_MESSAGE_LEN 1024 /* probably enough */

struct Error {
	struct location loc;
	enum error_level {
		ERR_NOTE, ERR_WARNING, ERR_FATAL
	} sev;
	char *msg;
};

struct reporter {
	struct Error *err;
	size_t num;
	bool pending, fatal;
};

struct reporter *new_reporter();
void error_push (struct reporter *r, struct location loc, enum error_level sev, char *fmt, ...);
void error_write(struct reporter *r, FILE *fp);
void error_clear(struct reporter *r);

#endif
