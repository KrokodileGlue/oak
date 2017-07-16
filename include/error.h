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

struct error_state {
	struct Error *err;
	size_t num;
	bool pending, fatal;
};

struct error_state *
error_new();

void
error_push (struct error_state *es, struct location loc, enum error_level sev, char *fmt, ...);

void
error_write(struct error_state *es, FILE *fp);

void
error_clear(struct error_state *es);

#endif
