#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"
#include "error.h"
#include "util.h"
#include "parser.h"
#include "ast-printer.h"

int do_file(char *filename)
{
	/* load */
	char *text = load_file(filename);
	if (!text) return EXIT_FAILURE;

	fprintf(stderr, "oak: input:\n%s\n", text);
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

	fprintf(stderr, "oak: now parsing...");

	/* parse */
	struct ParseState *ps = parser_new(tok);
	struct Statement **module = parse(ps);

	if (ps->es->fatal) {
		fprintf(stderr, "\n");
		error_write(ps->es, stderr);
		parser_clear(ps);
		free_ast(module);
		goto error;
	} else if (ps->es->pending) {
		error_write(ps->es, stderr);
	}

	print_ast(stderr, module);
	parser_clear(ps);
	free_ast(module);

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
