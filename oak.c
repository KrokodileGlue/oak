#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"
#include "error.h"
#include "util.h"
#include "parser.h"
#include "symbol.h"
#include "module.h"

int do_file(char *filename)
{
	/* load */
	char *text = load_file(filename);
	if (!text) return EXIT_FAILURE;

	DOUT("oak: input:\n%s\n", text);
	DOUT("oak: now lexing...\n");

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

	DOUT("oak: now parsing...");

	/* parse */
	struct ParseState *ps = parser_new(tok);
	struct Module *module = parse(ps);

	if (ps->es->fatal) {
		DOUT("\n");
		error_write(ps->es, stderr);
		parser_clear(ps);
		goto error;
	} else if (ps->es->pending) {
		error_write(ps->es, stderr);
	}

	print_ast(stderr, module->tree);
	parser_clear(ps);

	DOUT("oak: generating symbol table...");

	/* symbolize */
	struct Symbolizer *si = mksymbolizer(module);
	struct Symbol *st = symbolize_module(si, module);
	print_symbol(stderr, 0, st);
	DOUT("\n");

	if (si->es->fatal) {
		error_write(si->es, stderr);
		symbolizer_free(si);
		goto error;
	} else if (si->es->pending) {
		error_write(si->es, stderr);
	}

	symbolizer_free(si);

	/* TODO: compile and run */

	free_ast(module->tree);
	module_free(module);
	token_clear(tok);
	free(text);
	return EXIT_SUCCESS;

error:
	token_clear(tok);
	free(text);
	return EXIT_FAILURE;
error2:
	free_ast(module->tree);
	token_clear(tok);
	free(text);
	return EXIT_FAILURE;
}

int main(int argc, char **argv)
{
	return do_file(argv[1]);
}
