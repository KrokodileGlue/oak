#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "error.h"
#include "util.h"

int main(int argc, char** argv)
{
	char* text = load_file(argv[1]);

	if (text) {
		struct Token* tok = tokenize(text, argv[1]);
		write_tokens(stderr, tok);
		delete_tokens(tok);
		check_errors(stderr);
		free(text);
		if (encountered_error()) exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
