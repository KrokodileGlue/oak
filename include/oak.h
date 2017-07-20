#ifndef OAK_H
#define OAK_H

#include <stdlib.h>
#include "module.h"

struct oak {
	struct module **modules;
	size_t num;

	bool print_input;
	bool print_tokens;
	bool print_ast;
	bool print_symbol_table;
	bool print_everything;
	bool print_anything;
};

struct oak *new_oak();
struct module *load_module(struct oak *k, char *path, char *name);
char *process_arguments(struct oak *k, int argc, char **argv);

#endif
