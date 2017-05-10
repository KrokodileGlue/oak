#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "error.h"
#include "util.h"
#include "parser.h"

int main(int argc, char **argv)
{
	char *text = load_file(argv[1]);

	if (text) {
		struct ErrorState *es = error_new();
		struct Token *tok = tokenize(es, text, argv[1]);

		token_write(tok, stderr);
		
		if (es->pending) {
			error_write(es, stderr);
		}

		if (es->fatal) {
			token_clear(tok);
			free(text);
			error_clear(es);
			exit(EXIT_FAILURE);
		}

		error_clear(es);
		es = error_new();

		/* parse stuff */
		struct Statement **module = parse(es, tok);
		
		if (es->pending) {
			error_write(es, stderr);
		}

		if (es->fatal) {
			token_clear(tok);
			free(text);
			error_clear(es);
			exit(EXIT_FAILURE);
		}

		token_clear(tok);
		free(text);
		error_clear(es);
	}

	return EXIT_SUCCESS;
}
