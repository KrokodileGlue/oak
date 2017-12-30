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
	size_t num_instr;

	struct statement **tree;
	size_t num_nodes;

	struct constant_table *ct;
	struct symbol *sym;

	struct statement *stmt;
	struct reporter *r;
	struct gc *gc;

	int **stack_base;
	int *stack_top;
	int sp;

	bool debug;
};

bool compile(struct module *m, bool debug);

#endif
