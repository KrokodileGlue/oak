#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdint.h>
#include "tree.h"
#include "module.h"
#include "oak.h"

struct symbol {
	uint64_t        id;
	char           *name;
	struct token   *tok;
	struct module  *module;

	struct symbol  *parent;
	struct symbol **children;
	size_t          num_children;
	size_t          num_variables;
	size_t          num_arguments;
	size_t          address;
	int             scope;
	bool            global;

	int next, last;
	int *label;
	int labelp;
	int fp;
	bool imp;

	enum symbol_type {
		SYM_FN,
		SYM_VAR,
		SYM_GLOBAL,
		SYM_BLOCK,
		SYM_ARGUMENT,
		SYM_MODULE,
		SYM_IMPORTED,
		SYM_LABEL,
		SYM_INVALID
	} type;
};

struct symbolizer {
	struct symbol   *symbol;
	struct reporter *r;
	struct oak      *k;

	int             *scope_stack;
	int              fp;

	/* The index of the top of the scope stack. */
	int              scope_pointer;
};

struct symbolizer *new_symbolizer(struct oak *k);
void symbolizer_free(struct symbolizer *si);
void free_symbol(struct symbol *sym);
bool symbolize_module(struct module *m, struct oak *k, struct symbol *parent);
void print_symbol(FILE *f, size_t depth, struct symbol *s);
struct symbol *resolve(struct symbol *sym, char *name);
struct symbol *find_from_scope(struct symbol *sym, int scope);
void set_next(struct symbol *sym, int next);
void set_last(struct symbol *sym, int last);

#endif
