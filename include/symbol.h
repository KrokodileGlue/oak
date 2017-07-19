#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdint.h>
#include "tree.h"
#include "module.h"

struct symbol {
	uint64_t        id;
	char           *name;
	struct token   *tok;
	struct module  *module;

	struct symbol  *parent;
	struct symbol **children;
	size_t          num_children;

	enum SymbolType {
		SYM_FN,
		SYM_VAR,
		SYM_CLASS,
		SYM_GLOBAL,
		SYM_BLOCK,
		SYM_ARGUMENT,
		SYM_MODULE,
		SYM_INVALID
	} type;
};

struct symbolizer {
	struct symbol *symbol;
	struct error_state *es;
};

struct symbolizer *
new_symbolizer();

void
symbolizer_free(struct symbolizer *si);

void
free_symbol(struct symbol *sym);

struct symbol *
symbolize_module(struct symbolizer *si, struct module *m);

void
print_symbol(FILE *f, size_t depth, struct symbol *s);

#endif
