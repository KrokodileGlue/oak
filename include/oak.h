#ifndef OAK_H
#define OAK_H

#include <stdlib.h>
#include "module.h"
#include "value.h"

struct oak {
	struct module **modules;
	uint16_t num;

	bool debug;

	bool print_input;
	bool print_tokens;
	bool print_ast;
	bool print_symbol_table;
	bool print_code;
	bool print_gc;
	bool print_vm;
	bool talkative;

	bool print_everything;
	bool print_anything;

	/*
	 * This is never decremented, it only serves to produce unique
	 * id numbers for blocks. It's placed here instead of in the
	 * symbolizer because
	 */
	int scope;

	struct value *stack;
	size_t sp;

	char *eval;
};

struct oak *new_oak();
void free_oak(struct oak *k);
struct module *load_module(struct oak *k, struct symbol *parent, char *text,
            char *path, char *name, struct vm *vm, int stack_base);
char *process_arguments(struct oak *k, int argc, char **argv);

#endif
