#ifndef MODULE_H
#define MODULE_H

#include <stdlib.h>
#include "tree.h"

struct Module {
	char *name;
	struct Symbol *st;

	struct Statement **tree;
	size_t num;
};

struct Module *mkmodule(char *file, struct Statement **tree, size_t num);
void module_free(struct Module *m);

#endif
