#include "module.h"
#include "util.h"
#include "token.h"
#include "symbol.h"

struct Module *mkmodule(char *file, struct Statement **tree, size_t num, struct Token *tok)
{
	struct Module *m = oak_malloc(sizeof *m);

	memset(m, 0, sizeof *m);
	m->name = strclone(file);
	chop_extension(m->name);
	m->tree = tree;
	m->num = num;
	token_rewind(&tok);
	m->tok = tok;

	return m;
}

void module_free(struct Module *m)
{
	token_clear(m->tok);
	free_ast(m->tree);
	free(m->name);
	free(m->text);
	free(m);
}
