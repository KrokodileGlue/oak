#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdint.h>
#include "tree.h"
#include "module.h"

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
		SYM_MODULE,
		SYM_INVALID
	} type;
};

struct Symbolizer {
	struct Symbol *symbol;
	struct ErrorState *es;
};

struct Symbolizer *mksymbolizer();
void symbolizer_free(struct Symbolizer *si);

struct Symbol *symbolize_module(struct Symbolizer *si, struct Module *m);
void print_symbol(FILE *f, size_t depth, struct Symbol *s);

#endif
