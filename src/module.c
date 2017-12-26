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
	m->gc = new_gc();

	return m;
}

void
free_module(struct module *m)
{
	if (m->stage >= MODULE_STAGE_LEXED)      token_clear(m->tok);
	if (m->stage >= MODULE_STAGE_PARSED)     free_ast(m->tree);

	free(m->text);
	free(m->name);
	free(m->path);

	free(m);
}
