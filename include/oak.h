#ifndef OAK_H
#define OAK_H

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
	int scope;

	struct value *stack;
	size_t sp;

	char *eval;
};

typedef struct oak oak;

oak *oak_new();
void oak_free(oak *k);
char *process_arguments(oak *k, int argc, char **argv);
struct module *oak_load_module(oak *k, char *path);
struct module *oak_load_child(oak *k, struct module *m, char *path);
struct module *oak_load_module2(oak *k, char *text);
struct module *oak_load_child2(oak *k, struct module *m, char *text);
void oak_print_modules(oak *k);
void oak_print_value(FILE *f, struct gc *gc, struct value v);

#endif
