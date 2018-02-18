#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "oak.h"
#include "lexer.h"
#include "error.h"
#include "util.h"
#include "parse.h"
#include "symbol.h"
#include "module.h"
#include "oak.h"
#include "compile.h"
#include "vm.h"

oak *
oak_new()
{
	oak *k = oak_malloc(sizeof *k);
	memset(k, 0, sizeof *k);
	k->talkative = true;
	return k;
}

void
oak_free(oak *k)
{
	for (int i = k->num - 1; i >= 0; i--)
		free_module(k->modules[i]);

	if (k->stack) free(k->stack);
	free(k->modules);
	free(k);
}

void
oak_print_modules(oak *k)
{
	print_modules(k);
}

struct module *
oak_load_module(oak *k, char *path)
{
	char *name = strclone(path);
	chop_extension(name);
	struct module *m = load_module(k, NULL, load_file(path), path, name, NULL, -1);
	free(name);
	return m;
}

struct module *
oak_load_child(oak *k, struct module *m, char *path)
{
	char *name = strclone(path);
	chop_extension(name);
	push_frame(m->vm);
	struct module *m2 = load_module(k, m->sym, load_file(path), path, name, m->vm, 0);
	free(name);
	return m2;
}

struct module *
oak_load_module2(oak *k, char *text)
{
	return load_module(k, NULL, text, "*e.k*", "e", NULL, -1);
}

struct module *
oak_load_child2(oak *k, struct module *m, char *text)
{
	push_frame(m->vm);
	return load_module(k, m->sym, text, "*e.k*", "*e*", m->vm, 0);
}

void
oak_print_value(FILE *f, struct gc *gc, struct value v)
{
	print_value(f, gc, v);
}
