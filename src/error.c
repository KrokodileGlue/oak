#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"
#include "util.h"
#include "color.h"

static char error_level_strs[][32] = { "note", "warning", "error" };

struct error_state *
error_new()
{
	struct error_state *es = oak_malloc(sizeof *es);

	*es = (struct error_state) {
		.err = NULL,      .num = 0,
		.pending = false, .fatal = false
	};

	return es;
}

void
error_push(struct error_state *es, struct location loc, enum error_level sev, char *fmt, ...)
{
	es->pending = true;
	es->err = realloc(es->err, sizeof *(es->err) * ++(es->num));

	char* msg = oak_malloc(ERR_MAX_MESSAGE_LEN + 1);

	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, ERR_MAX_MESSAGE_LEN, fmt, args);
	va_end(args);

	es->err[es->num - 1] = (struct Error){loc, sev, msg};
	if (sev == ERR_FATAL) es->fatal = true;
}

void
error_write(struct error_state *es, FILE *fp)
{
	struct Error *errors = es->err;
	for (size_t i = 0; i < es->num; i++) {
		struct Error err = errors[i];

		fprintf(fp, ERROR_LOCATION_COLOR"%s:%zd:%zd: "RESET_COLOR, err.loc.file, line_number(err.loc), column_number(err.loc));
		fprintf(fp, ERROR_MSG_COLOR"%s:"RESET_COLOR" %s\n", error_level_strs[err.sev], err.msg);

		char *line = get_line(err.loc);
		fprintf(fp, "\t%.*s", (int)index_in_line(err.loc), line);
		fprintf(fp, ERROR_HIGHLIGHT_COLOR"%.*s"RESET_COLOR, (int)err.loc.len, index_in_line(err.loc) + line);
		fprintf(fp, "%s\n\t", err.loc.len <= strlen(line) - index_in_line(err.loc)
			? line + index_in_line(err.loc) + err.loc.len
			: ERROR_MSG_COLOR" [token truncated]"RESET_COLOR);

		for (size_t j = 0; j < index_in_line(err.loc); j++) {
			fputc(line[j] == '\t' ? '\t' : ' ', fp);
		}

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
error_clear(struct error_state *es)
{
	for (size_t i = 0; i < es->num; i++) {
		error_delete(es->err[i]);
	}
	free(es->err);
	free(es);
}
