#ifndef COMPILE_H
#define COMPILE_H

#include <stdbool.h>
#include "constant.h"
#include "module.h"
#include "value.h"
#include "oak.h"
#include "symbol.h"

struct compiler {
	struct instruction *code;
	size_t num_instr;

	struct statement **tree;
	size_t num_nodes;

	struct constant_table table;
	struct symbol *sym;

	struct statement *stmt;
	struct reporter *r;

	int *stack_base;
	int *stack_top;
	int sp;
};

void print_constant_table(FILE *f, struct constant_table table);
bool compile(struct module *m);

#endif
