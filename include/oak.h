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
};

struct oak *
new_oak();

char *
process_arguments(int argc, char **argv, struct oak *k);

#endif
