#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"
#include "util.h"
#include "color.h"

static char error_level_strs[][32] = { "note", "warning", "error" };

struct ErrorState *error_new()
{
	struct ErrorState *es = oak_malloc(sizeof *es);

	*es = (struct ErrorState) {
		.err = NULL,      .num = 0,
		.pending = false, .fatal = false
	};

	return es;
}

void error_push(struct ErrorState *es, struct Location loc, enum ErrorLevel sev, char *fmt, ...)
{
	es->pending = true;
	es->err = (es-> err == NULL)
		? oak_malloc(sizeof *(es->err) * ++(es->num))
		: realloc(es->err, sizeof *(es->err) * ++(es->num));

	char* msg = oak_malloc(ERR_MAX_MESSAGE_LEN + 1);

	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, ERR_MAX_MESSAGE_LEN, fmt, args);
	va_end(args);

	es->err[es->num - 1] = (struct Error){loc, sev, msg};
	if (sev == ERR_FATAL) es->fatal = true;
}

void error_write(struct ErrorState *es, FILE *fp)
{
	struct Error *errors = es->err;
	for (size_t i = 0; i < es->num; i++) {
		struct Error err = errors[i];

		fprintf(fp, ERROR_LOCATION_COLOR"%s:%zd:%zd: "RESET_COLOR, err.loc.file, line_number(err.loc), column_number(err.loc));
		fprintf(fp, ERROR_MSG_COLOR"%s:"RESET_COLOR" %s\n", error_level_strs[err.sev], err.msg);

		char *line = get_line(err.loc);
		fprintf(fp, "\t%.*s", index_in_line(err.loc), line);
		fprintf(fp, ERROR_HIGHLIGHT_COLOR"%.*s"RESET_COLOR, err.loc.len, index_in_line(err.loc) + line);
		fprintf(fp, "%s\n\t", err.loc.len <= strlen(line) - index_in_line(err.loc)
			? line + index_in_line(err.loc) + err.loc.len
			: ERROR_MSG_COLOR" [token truncated]"RESET_COLOR);

		for (size_t j = 0; j < index_in_line(err.loc); j++) {
			fputc(line[j] == '\t' ? '\t' : ' ', fp);
		}

		fprintf(fp, ERROR_HIGHLIGHT_COLOR);
		fputc('^', fp);

		size_t j = 1;
		size_t len = (err.loc.len > strlen(line) - index_in_line(err.loc))
			? strlen(line) - index_in_line(err.loc)
			: err.loc.len;
		while (j++ < len) fputc('~', fp);
		fprintf(fp, RESET_COLOR);

		free(line);

		fputc('\n', fp);
	}
}

static void error_delete(struct Error err)
{
	/* TODO: investigate how i was retarded enough to write this */
//	free(err.msg);
}

void error_clear(struct ErrorState *es)
{
	for (size_t i = 0; i < es->num; i++) {
		error_delete(es->err[i]);
	}
	free(es->err);
	free(es);
}
