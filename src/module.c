#include "module.h"
#include "util.h"
#include "token.h"
#include "symbol.h"

struct module *
new_module(char *file, struct statement **tree, size_t num, struct token *tok)
{
	struct module *m = oak_malloc(sizeof *m);

	memset(m, 0, sizeof *m);
	m->name = strclone(file);
	chop_extension(m->name);
	m->tree = tree;
	m->num = num;
	token_rewind(&tok);
	m->tok = tok;

	return m;
}

void
module_free(struct module *m)
{
	token_clear(m->tok);
	free_ast(m->tree);
	free(m->name);
	free(m->text);
	free(m);
}
