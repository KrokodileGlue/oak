#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "error.h"
#include "util.h"

int main(int argc, char** argv)
{
	char* text = load_file(argv[1]);

	if (text) {
		struct ErrorState* es;
		error_new(&es);
		struct Token* tok = tokenize(es, text, argv[1]);

		token_write(tok, stderr);
		token_clear(tok);

		if (es->pending) {
			error_write(es, stderr);
		}
		free(text);

		if (es->fatal) {
			error_clear(es);
			exit(EXIT_FAILURE);
		}
		error_clear(es);
	}

	return EXIT_SUCCESS;
}
