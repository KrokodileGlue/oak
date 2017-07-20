#include "module.h"
#include "util.h"
#include "token.h"
#include "symbol.h"

struct module *
new_module(char *path)
{
	struct module *m = oak_malloc(sizeof *m);

	memset(m, 0, sizeof *m);
	m->path = strclone(path);
	m->stage = MODULE_STAGE_EMPTY;

	return m;
}

void
free_module(struct module *m)
{
	token_clear(m->tok);
	free_ast(m->tree);
	free(m->name);
	free(m->text);
	free(m);
}
