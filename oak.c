#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "error.h"
#include "util.h"

int main(int argc, char** argv)
{
	char* text = load_file(argv[1]);

	if (text) {
		struct ErrorState* es = malloc(sizeof *es);
		*es = (struct ErrorState){
			.err = NULL,      .num = 0,
			.pending = false, .fatal = false};
		struct Token* tok = tokenize(es, text, argv[1]);

		token_write(tok, stderr);
		token_clear(tok);

		if (es->pending) {
			error_write(es, stderr);
			error_clear(es);
		}
		free(text);

		if (es->fatal) exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
