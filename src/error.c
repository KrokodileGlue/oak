#include <stdlib.h>
#include <stdio.h>

#include "error.h"
#include "util.h"

struct Error {
	char* str;
	char* message;
	size_t location;
};

static struct Error* errors = NULL;
static size_t num_errors = 0;

void push_error(char* str, size_t location, char* message)
{
	if (!errors) errors = malloc(sizeof (struct Error));
	else errors = realloc(errors, sizeof (struct Error) * (num_errors + 1));

	errors[num_errors].str = str;
	errors[num_errors].message = message;
	errors[num_errors++].location = location;
}

void write_errors(FILE* fp)
{
	for (size_t i = 0; i < num_errors; i++) {
		fprintf(fp, "%zd:%zd: %s\n", line_number(errors[i].str, errors[i].location),
			column_number(errors[i].str, errors[i].location), errors[i].message);
		fprintf(fp, "\t%s\n", get_line(errors[i].str, errors[i].location));
	}
}
