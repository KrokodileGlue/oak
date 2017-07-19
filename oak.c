#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"
#include "error.h"
#include "util.h"
#include "parser.h"
#include "symbol.h"
#include "module.h"
#include "oak.h"

struct oak *new_oak()
{
	struct oak *k = oak_malloc(sizeof *k);

	memset(k, 0, sizeof *k);

	return k;
}

char *
process_arguments(int argc, char **argv, struct oak *k)
{
	char *filename = NULL;

	for (int i = 1; i < argc; i++) {
		if (!k->print_input)        k->print_input        = !strcmp(argv[i], "-pi");
		if (!k->print_tokens)       k->print_tokens       = !strcmp(argv[i], "-pt");
		if (!k->print_ast)          k->print_ast          = !strcmp(argv[i], "-pa");
		if (!k->print_symbol_table) k->print_symbol_table = !strcmp(argv[i], "-ps");
		if (!k->print_everything)   k->print_everything   = !strcmp(argv[i], "-p");

		if (argv[i][0] != '-')  {
			if (filename) {
				DOUT("received multiple input files; '%s' and '%s'\n", filename, argv[i]);
				exit(EXIT_FAILURE);
			} else {
				filename = argv[i];
			}
		}
	}

	if (k->print_everything) {
		k->print_input        = true;
		k->print_tokens       = true;
		k->print_ast          = true;
		k->print_symbol_table = true;
	}

	if (!filename) {
		DOUT("oak: did not receive an input file\n");
		exit(EXIT_FAILURE);
	}

	return filename;
}

void
do_file(struct oak *k, char *filename)
{
	/* load */
	char *text = load_file(filename);
	if (!text)
		DOUT("oak: could not load input file %s\n", filename);

	struct module *m = new_module(filename);
	char *name = strclone(filename);
	chop_extension(name);
	m->name = name;
	m->text = text;

	if (k->print_input)
		DOUT("oak: input:\n%s\n", text);

	/* lex */
	struct lexer *ls = new_lexer(text, filename);
	struct token *tok = tokenize(ls);
	if (!tok) goto error;

	m->tok = tok;

	if (k->print_tokens)
		token_write(tok, stderr);
	if (ls->es->fatal) {
		error_write(ls->es, stderr);
		lexer_clear(ls);
		goto error;
	} else if (ls->es->pending) {
		error_write(ls->es, stderr);
	}

	lexer_clear(ls);

	/* parse */
	struct parser *ps = new_parser(tok);
	if (!parse(m)) goto error;

	if (k->print_ast) print_ast(stderr, m->tree);
	parser_clear(ps);

	/* symbolize */
	struct symbolizer *si = new_symbolizer(m);
	struct symbol *st = symbolize_module(si, m);
	if (!st) goto error;
	if (k->print_symbol_table) {
		print_symbol(stderr, 0, st);
		DOUT("\n");
	}

	symbolizer_free(si);

	/* TODO: compile and run */
error:
	token_clear(tok);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	struct oak *k = new_oak();

	char *filename = process_arguments(argc, argv, k);
	do_file(k, filename);

	return EXIT_SUCCESS;
}
