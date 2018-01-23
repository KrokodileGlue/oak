#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"
#include "util.h"
#include "color.h"

static const char *error_level_strs[] = { "note", "warning", "error", "killed" };

struct reporter *
new_reporter()
{
	struct reporter *r = oak_malloc(sizeof *r);

	*r = (struct reporter) {
		.err = NULL,      .num = 0,
		.pending = false, .fatal = false
	};

	return r;
}

void
error_push(struct reporter *r, struct location loc, enum error_level sev, char *fmt, ...)
{
	r->pending = true;
	r->err = realloc(r->err, sizeof *(r->err) * ++(r->num));

	char* msg = oak_malloc(ERR_MAX_MESSAGE_LEN + 1);

	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, ERR_MAX_MESSAGE_LEN, fmt, args);
	va_end(args);

	r->err[r->num - 1] = (struct Error){loc, sev, msg};
	if (sev == ERR_FATAL) r->fatal = true;
}

void
error_write(struct reporter *r, FILE *fp)
{
	struct Error *errors = r->err;

	for (size_t i = 0; i < r->num; i++) {
		struct Error err = errors[i];

		fprintf(fp, ERROR_LOCATION_COLOR"%s:%zd:%zd: "RESET_COLOR, err.loc.file, line_number(err.loc), column_number(err.loc));
		fprintf(fp, ERROR_MSG_COLOR"%s:"RESET_COLOR" %s\n", error_level_strs[err.sev], err.msg);

		char *line = get_line(err.loc);
		fprintf(fp, "\t%.*s", (int)index_in_line(err.loc), line);
		fprintf(fp, ERROR_HIGHLIGHT_COLOR"%.*s"RESET_COLOR, (int)err.loc.len, index_in_line(err.loc) + line);
		fprintf(fp, "%s\n\t", err.loc.len <= strlen(line) - index_in_line(err.loc)
			? line + index_in_line(err.loc) + err.loc.len
			: ERROR_MSG_COLOR" [token truncated]"RESET_COLOR);

		for (size_t j = 0; j < index_in_line(err.loc); j++)
			fputc(line[j] == '\t' ? '\t' : ' ', fp);

		fprintf(fp, "%s", ERROR_HIGHLIGHT_COLOR);
		fputc('^', fp);

		size_t j = 1;
		size_t len = (err.loc.len > strlen(line) - index_in_line(err.loc))
			? strlen(line) - index_in_line(err.loc)
			: err.loc.len;
		while (j++ < len) fputc('~', fp);
		fprintf(fp, "%s", RESET_COLOR);

		free(line);

		fputc('\n', fp);
	}
}

static void
error_delete(struct Error err)
{
	free(err.msg);
}

void
error_clear(struct reporter *r)
{
	for (size_t i = 0; i < r->num; i++)
		error_delete(r->err[i]);

	free(r->err);
	free(r);
}
