#ifndef COMPILE_H
#define COMPILE_H

#include <stdbool.h>
#include "constant.h"
#include "module.h"
#include "value.h"

struct compiler {
	struct instruction *code;
	size_t num_instr;

	struct statement **tree;
	size_t num_nodes;

	struct constant_table table;

	struct symbol *sym;
};

void print_constant_table(FILE *f, struct constant_table table);
bool compile(struct module *m);

#endif
