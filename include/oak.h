#ifndef OAK_H
#define OAK_H

#include <stdlib.h>
#include "module.h"

struct oak {
	struct module **modules;
	size_t num;

	bool debug;

	bool print_input;
	bool print_tokens;
	bool print_ast;
	bool print_symbol_table;
	bool print_code;

	bool print_everything;
	bool print_anything;
};

struct oak *new_oak();
void free_oak(struct oak *k);
struct module *load_module(struct oak *k, char *path, char *name);
char *process_arguments(struct oak *k, int argc, char **argv);

#endif
