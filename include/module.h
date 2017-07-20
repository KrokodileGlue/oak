#ifndef MODULE_H
#define MODULE_H

#include <stdlib.h>
#include "tree.h"

struct module {
	char *name, *text, *path;
	struct symbol *sym;
	struct token *tok; /* keep the token stream for later free()ing */

	struct statement **tree;
	size_t num;

	enum {
		MODULE_STAGE_EMPTY,
		MODULE_STAGE_LEXED,
		MODULE_STAGE_PARSED,
		MODULE_STAGE_SYMBOLIZED,
		MODULE_STAGE_COMPILED
	} stage;
};

struct module *new_module(char *path);

void module_free(struct module *m);

#endif
