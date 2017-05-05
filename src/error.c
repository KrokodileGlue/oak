#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"
#include "util.h"

static char error_level_strs[][32] = { "note", "warning", "error" };
#define ERR_MAX_MESSAGE_LEN 1024 /* probably enough */

void error_new(struct ErrorState** es)
{
	*es = malloc(sizeof **es);

	**es = (struct ErrorState) {
		.err = NULL,      .num = 0,
		.pending = false, .fatal = false
	};
}

void error_push(struct ErrorState* es, struct Location loc, enum ErrorLevel sev, char* fmt, ...)
{
	es->pending = true;
	es->err = (es-> err == NULL)
		? malloc(sizeof *(es->err) * ++(es->num))
		: realloc(es->err, sizeof *(es->err) * ++(es->num));

	char* msg = malloc(ERR_MAX_MESSAGE_LEN + 1);

	if (fmt) {
		va_list args;
		va_start(args, fmt);
		vsnprintf(msg, ERR_MAX_MESSAGE_LEN, fmt, args);
		va_end(args);
	}

	es->err[es->num - 1] = (struct Error){loc, sev, msg};
	if (sev == ERR_FATAL) es->fatal = true;
}

void error_write(struct ErrorState* es, FILE* fp)
{
	struct Error* errors = es->err;
	for (size_t i = 0; i < es->num; i++) {
		struct Error err = errors[i];
		
		fprintf(fp, "%s:%zd:%zd: ", err.loc.file, line_number(err.loc), column_number(err.loc));
		fprintf(fp, "%s: %s\n", error_level_strs[err.sev], err.msg);

		char* line = get_line(err.loc);
		fprintf(fp, "\t%s\n\t", line);
		
		for (size_t j = 0; j < index_in_line(err.loc); j++) {
			fputc(line[j] == '\t' ? '\t' : ' ', fp);
		}
		
		free(line);

		fputc('^', fp);

		size_t j = 1;
		while (j++ < err.loc.len && err.loc.len != (size_t)-1)
			fputc('~', fp);

		fputc('\n', fp);
	}
}

static void error_delete(struct Error err)
{
	free(err.msg);
}

void error_clear(struct ErrorState* es)
{
	for (size_t i = 0; i < es->num; i++) {
		error_delete(es->err[i]);
	}
	free(es->err);
	free(es);
}
