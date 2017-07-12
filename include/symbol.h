#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdint.h>
#include "tree.h"

struct Symbol {
	uint64_t id;
	char *name;
	struct Token *tok;

	struct Symbol *parent;
	struct Symbol **children;
	size_t num_children;

	enum SymbolType {
		SYM_FN,
		SYM_VAR,
		SYM_CLASS,
		SYM_GLOBAL,
		SYM_BLOCK,
		SYM_ARGUMENT,
		SYM_INVALID
	} type;
};

struct Symbolizer {
	struct Symbol *symbol;
	struct Statement **m; /* the module we're working on */
	size_t i;
};

struct Symbolizer *mksymbolizer();
struct Symbol *symbolize_module(struct Symbolizer *si);
void print_symbol(FILE *f, size_t depth, struct Symbol *s);

#endif
