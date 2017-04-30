#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"
#include "util.h"

struct Error {
	struct Location loc;
	enum ErrorLevel level;
	char* message;
	size_t len;
};

static struct Error* errors = NULL;
static size_t num_errors = 0;
static bool should_die = false;

static char error_level_strs[][32] = { "note", "warning", "error" };

#define MAX_ERROR_MESSAGE_LEN 512 /* probably enough */

void push_error(struct Location loc, enum ErrorLevel level, size_t len, char* fmt, ...)
{
	if (!errors) errors = malloc(sizeof (struct Error));
	else errors = realloc(errors, sizeof (struct Error) * (num_errors + 1));

	char* message = malloc(MAX_ERROR_MESSAGE_LEN + 1);

	if (fmt) {
		va_list args;
		va_start(args, fmt);
		vsnprintf(message, MAX_ERROR_MESSAGE_LEN, fmt, args);
		va_end(args);
	}
	
	errors[num_errors++] = (struct Error){loc, level, message, len};
	if (level == ERR_FATAL) should_die = true;
}

void check_errors(FILE* fp)
{
	write_errors(fp);
	if (should_die) exit(EXIT_FAILURE);
}

void write_errors(FILE* fp)
{
	for (size_t i = 0; i < num_errors; i++) {
		char* line = get_line(errors[i].loc);
		fprintf(fp, "%s:%zd:%zd: %s: %s\n", errors[i].loc.file, line_number(errors[i].loc), column_number(errors[i].loc), error_level_strs[errors[i].level], errors[i].message);
		fprintf(fp, "\t%s\n\t", line);
		for (size_t j = 0; j < index_in_line(errors[i].loc); j++) {
			fputc(line[j] == '\t' ? '\t' : ' ', fp);
		}
		fputc('^', fp);
		size_t j = 0;
		while (j < (size_t)((errors[i].len <= 1) ? 1 : errors[i].len - 1)) {
			fputc('~', fp);
			j++;
		}
		fputc('\n', fp);
		free(line);
	}
}
