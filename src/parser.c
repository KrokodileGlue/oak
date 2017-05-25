#include "parser.h"
#include "error.h"
#include "util.h"
#include "operator.h"

#include <stdbool.h>

struct ParseState *parser_new(struct Token *tok)
{
	struct ParseState *ps = oak_malloc(sizeof *ps);

	ps->es = error_new();
	ps->tok = tok;

	return ps;
}

void parser_clear(struct ParseState *ps)
{
	error_clear(ps->es);
	/* the parser is not responsible for the token stream, so we
	 * don't have to free it here. */
	free(ps);
}

struct Statement **parse(struct ParseState *ps)
{
	size_t num_stmts = 1;
	struct Statement **program = oak_malloc(sizeof *program * num_stmts);

	while (ps->tok) {
		switch (ps->tok->type) {
		default: 
			ps->tok = ps->tok->next;
		}
	}

	return program;
}
