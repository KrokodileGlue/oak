#include "module.h"
#include "util.h"

struct Module *mkmodule(char *file, struct Statement **tree, size_t num)
{
	struct Module *m = oak_malloc(sizeof *m);

	memset(m, 0, sizeof *m);
	m->name = strclone(file);
	chop_extension(m->name);
	m->tree = tree;
	m->num = num;

	return m;
}

void module_free(struct Module *m)
{
	free(m->name);
	free(m);
}
