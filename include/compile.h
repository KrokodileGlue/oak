#ifndef COMPILE_H
#define COMPILE_H

#include <stdbool.h>
#include "constant.h"
#include "module.h"
#include "value.h"
#include "oak.h"
#include "symbol.h"
#include "gc.h"

struct compiler {
	struct instruction *code;
	size_t ip;
	size_t instr_alloc;

	size_t *next;
	size_t *last;
	int np, lp;

	struct statement **tree;
	size_t num_nodes;

	struct constant_table *ct;
	struct symbol *sym;

	struct module *m;
	struct statement *stmt;
	struct statement *loop;
	struct reporter *r;
	struct gc *gc;

	int *stack_base;
	int *stack_top;
	int *var;
	int sp;

	bool eval;
	bool debug;
	bool in_expr;
};

bool compile(struct module *m, struct constant_table *ct, struct symbol *sym,
        bool debug, bool eval, int stack_base);

#endif
