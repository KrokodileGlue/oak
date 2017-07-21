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

struct oak *
new_oak()
{
	struct oak *k = oak_malloc(sizeof *k);

	memset(k, 0, sizeof *k);

	return k;
}

void free_oak(struct oak *k)
{
	for (size_t i = 0; i < k->num; i++) {
		free_module(k->modules[i]);
	}

	free(k->modules);
	free(k);
}

char *
process_arguments(struct oak *k, int argc, char **argv)
{
	char *path = NULL;

	for (int i = 1; i < argc; i++) {
		if (!k->print_input)        k->print_input        = !strcmp(argv[i], "-pi");
		if (!k->print_tokens)       k->print_tokens       = !strcmp(argv[i], "-pt");
		if (!k->print_ast)          k->print_ast          = !strcmp(argv[i], "-pa");
		if (!k->print_symbol_table) k->print_symbol_table = !strcmp(argv[i], "-ps");
		if (!k->print_everything)   k->print_everything   = !strcmp(argv[i], "-p");

		if (argv[i][0] != '-')  {
			if (path) {
				DOUT("received multiple input files; '%s' and '%s'", path, argv[i]);
				exit(EXIT_FAILURE);
			} else {
				path = argv[i];
			}
		}
	}

	k->print_anything = (k->print_input || k->print_tokens || k->print_ast || k->print_symbol_table || k->print_everything);

	if (!path) {
		DOUT("did not receive an input file");
		exit(EXIT_FAILURE);
	}

	return path;
}

void add_module(struct oak *k, struct module *m)
{
	k->modules = oak_realloc(k->modules, sizeof k->modules[0] * (k->num + 1));
	k->modules[k->num++] = m;
}

void print_modules(struct oak *k)
{
	if (k->print_anything) {
		for (size_t i = 0; i < k->num; i++) {
			struct module *m = k->modules[i];

			fprintf(stderr, "============================== module '%s' ==============================\n", m->name);

			if (m->stage >= MODULE_STAGE_EMPTY
			    && (k->print_input || k->print_everything))
				DOUT("%s", m->text);

			if (m->stage >= MODULE_STAGE_LEXED
			    && (k->print_tokens || k->print_everything))
				token_write(m->tok, stderr);

			if (m->stage >= MODULE_STAGE_PARSED
			    && (k->print_ast || k->print_everything))
				print_ast(stderr, m->tree);

			if (m->stage >= MODULE_STAGE_SYMBOLIZED
			    && (k->print_symbol_table || k->print_everything)) {
				print_symbol(stderr, 0, m->sym);
				fputc('\n', stderr);
			}
		}
	}
}

struct module *
load_module(struct oak *k, char *path, char *name)
{
	for (size_t i = 0; i < k->num; i++)
		if (!strcmp(k->modules[i]->path, path))
			return k->modules[i];

	char *text = load_file(path);
	if (!text) return NULL;

	struct module *m = new_module(path);
	m->name = strclone(name);
	m->text = text;

	add_module(k, m);

	if (!tokenize(m)) return NULL;
	if (!parse(m)) return NULL;
	if (!symbolize_module(m, k)) return NULL;

	/* TODO: compile and run */
	return m;
}

int
main(int argc, char **argv)
{
	struct oak *k = new_oak();

	char *path = process_arguments(k, argc, argv);
	load_module(k, path, "*main*");
	print_modules(k);
	free_oak(k);

	return EXIT_SUCCESS;
}
