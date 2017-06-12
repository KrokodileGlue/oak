#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"
#include "error.h"
#include "util.h"
#include "parser.h"

int do_file(char *filename)
{
	/* load */
	char *text = load_file(filename);
	if (!text) return EXIT_FAILURE;

	fprintf(stderr, "oak: now lexing...\n");

	/* lex */
	struct LexState *ls = lexer_new(text, filename);
	struct Token *tok = tokenize(ls);

	token_write(tok, stderr); /* for debugging */

	if (ls->es->fatal) {
		error_write(ls->es, stderr);
		lexer_clear(ls);
		goto error;
	} else if (ls->es->pending) {
		error_write(ls->es, stderr);
	}

	lexer_clear(ls);

	fprintf(stderr, "oak: now parsing...\n");

	/* parse */
	struct ParseState *ps = parser_new(tok);
	struct Statement **module = parse(ps);

	if (ps->es->fatal) {
		error_write(ps->es, stderr);
		parser_clear(ps);
		goto error;
	} else if (ps->es->pending) {
		error_write(ps->es, stderr);
	}

	parser_clear(ps);

	/* TODO: compile and run */

	token_clear(tok);
	free(text);
	return EXIT_SUCCESS;

error:
	token_clear(tok);
	free(text);
	return EXIT_FAILURE;
}

int main(int argc, char **argv)
{
	return do_file(argv[1]);
}
