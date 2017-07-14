#ifndef MODULE_H
#define MODULE_H

#include <stdlib.h>
#include "tree.h"

struct Module {
	char *name, *text;
	struct Symbol *st;
	struct Token *tok; /* keep the token stream for later free()ing */

	struct Statement **tree;
	size_t num;
};

struct Module *mkmodule(char *file, struct Statement **tree, size_t num, struct Token *tok);
void module_free(struct Module *m);

#endif
