#ifndef MODULE_H
#define MODULE_H

#include <stdlib.h>
#include "tree.h"

struct module {
	char *name, *text, *path;
	struct symbol *st;
	struct token *tok; /* keep the token stream for later free()ing */

	struct statement **tree;
	size_t num;
};

struct module *
new_module(char *path);

void
module_free(struct module *m);

#endif
