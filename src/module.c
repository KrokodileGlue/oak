#include "module.h"
#include "util.h"
#include "token.h"
#include "symbol.h"
#include "vm.h"

struct module *
new_module(char *text, char *path)
{
	struct module *m = oak_malloc(sizeof *m);

	memset(m, 0, sizeof *m);
	m->path = strclone(path);
	m->stage = MODULE_STAGE_EMPTY;
	m->gc = new_gc();
	m->text = text;

	return m;
}

void
free_module(struct module *m)
{
	if (m->k->print_gc) DOUT("freeing module %s", m->name);

	if (m->stage >= MODULE_STAGE_LEXED)      token_clear(m->tok);
	if (m->stage >= MODULE_STAGE_PARSED)     free_ast(m->tree);
	if (m->stage >= MODULE_STAGE_SYMBOLIZED) if (!m->child) free_symbol(m->sym);

	if (!m->child) free_gc(m->gc);
	if (!m->child) free_constant_table(m->ct);

	free_vm(m->vm);
	free(m->text);
	free(m->name);
	free(m->path);
	free(m->code);

	free(m);
}
