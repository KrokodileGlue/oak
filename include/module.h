#ifndef MODULE_H
#define MODULE_H

#include <stdlib.h>

#include "tree.h"
#include "code.h"
#include "constant.h"
#include "gc.h"

struct module {
	char *name, *text, *path;
	struct symbol *sym;
	struct token *tok;

	struct statement **tree;
	size_t num_nodes;

	struct instruction *code;
	size_t num_instr;

	struct constant_table *ct;
	struct gc *gc;

	struct vm *vm;
	struct oak *k;

	uint16_t id;
	bool child;

	enum {
		MODULE_STAGE_EMPTY,
		MODULE_STAGE_LEXED,
		MODULE_STAGE_PARSED,
		MODULE_STAGE_SYMBOLIZED,
		MODULE_STAGE_COMPILED
	} stage;
};

struct module *new_module(char *text, char *path);
void free_module(struct module *m);

#endif
