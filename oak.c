#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"
#include "error.h"
#include "util.h"
#include "parse.h"
#include "symbol.h"
#include "module.h"
#include "oak.h"
#include "compile.h"
#include "vm.h"

struct oak *
new_oak()
{
	struct oak *k = oak_malloc(sizeof *k);
	memset(k, 0, sizeof *k);
	return k;
}

void free_oak(struct oak *k)
{
	for (int i = k->num - 1; i >= 0; i--)
		free_module(k->modules[i]);

	if (k->stack) free(k->stack);
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
		if (!k->print_code)         k->print_code         = !strcmp(argv[i], "-pc");
		if (!k->print_gc)           k->print_gc           = !strcmp(argv[i], "-pg");
		if (!k->debug)              k->debug              = !strcmp(argv[i], "-d");
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

	k->print_anything = (k->print_input || k->print_tokens || k->print_ast || k->print_symbol_table || k->print_code || k->print_everything);
	if (k->print_everything) k->print_code = true;
	if (k->print_everything) k->print_gc = true;

	if (!path) {
		DOUT("did not receive an input file");
		exit(EXIT_FAILURE);
	}

	return path;
}

void add_module(struct oak *k, struct module *m)
{
	k->modules = oak_realloc(k->modules, sizeof k->modules[0] * (k->num + 1));
	m->id = k->num;
	k->modules[k->num++] = m;
}

void print_modules(struct oak *k)
{
	if (k->print_anything) {
		for (size_t i = 0; i < k->num; i++) {
			struct module *m = k->modules[i];

			if (i != 0) fputc('\n', stderr);
			fprintf(stderr, "============================== module `%s' ==============================", m->name);

			if (m->stage >= MODULE_STAGE_EMPTY
			    && (k->print_input || k->print_everything))
				fprintf(stderr, "\n%s", m->text);

			if (m->stage >= MODULE_STAGE_LEXED
			    && (k->print_tokens || k->print_everything))
				token_write(m->tok, stderr);

			if (m->stage >= MODULE_STAGE_PARSED
			    && (k->print_ast || k->print_everything))
				print_ast(stderr, m->tree);

			if (m->stage >= MODULE_STAGE_SYMBOLIZED
			    && (k->print_symbol_table || k->print_everything)) {
				if (m->child)
					fprintf(stderr,
					        "\nnot printing symbol table - module is a child of `%s'.",
					        m->parent->name);
				else print_symbol(stderr, 0, m->sym);
			}

			if (m->stage >= MODULE_STAGE_COMPILED
			    && (k->print_code || k->print_everything)) {
				if (m->child)
					fprintf(stderr,
					        "\nnot printing constant table - module is a child of `%s'.",
					        m->parent->name);
				else print_constant_table(stderr, m->gc, m->ct);
				print_code(stderr, m->code, m->num_instr);
			}
		}

		fputc('\n', stderr);
	}
}

struct module *
load_module(struct oak *k, struct symbol *parent, char *text,
            char *path, char *name, struct vm *vm, int stack_base)
{
	for (size_t i = 0; i < k->num; i++)
		if (!strcmp(k->modules[i]->path, path) && strncmp(name, "*eval", 5))
			return k->modules[i];

	struct module *m = new_module(text, path);

	if (vm) {
		m->child = true;
		m->parent = vm->m;
		free_gc(m->gc);
		m->gc = vm->gc;
	}

	m->name = strclone(name);
	m->text = text;
	m->k = k;
	if (k->print_gc) m->gc->debug = true;

	add_module(k, m);

	if (!tokenize(m)) return NULL;
	if (!parse(m)) return NULL;
	if (!symbolize_module(m, k, parent)) return NULL;
	while (m->sym->parent) m->sym = m->sym->parent;
	if (!compile(m, vm ? vm->m->ct : NULL, parent ? parent : m->sym,
	             k->print_code, !!vm, vm ? stack_base : -1)) return NULL;

	if (vm) {
		struct constant_table *ct = vm->ct;
		struct instruction *c = vm->code;
		struct module *m2 = vm->m;
		int ip = vm->ip;

		vm->ct = m->ct;
		vm->code = m->code;
		vm->m = m;
		m->vm = vm;

		if (!k->debug) execute(m->vm, 0);
		m->vm = NULL;

		vm->code = c;
		vm->m = m2;
		vm->ip = ip;
		vm->ct = ct;
	} else {
		m->vm = new_vm(m, k, k->print_code);
		push_frame(m->vm);
		if (!k->debug) execute(m->vm, 0);
	}

	return m;
}

int
main(int argc, char **argv)
{
	struct oak *k = new_oak();

	char *path = process_arguments(k, argc, argv);
	load_module(k, NULL, load_file(path), path, "*main*", NULL, 0);
	print_modules(k);
	free_oak(k);

	return EXIT_SUCCESS;
}
