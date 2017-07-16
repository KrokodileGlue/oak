#ifndef PARSER_H
#define PARSER_H

#include "tree.h"
#include "token.h"
#include "error.h"

struct parser {
	struct error_state *es;
	struct token *tok;
};

struct parser *
parser_new();

void
parser_clear(struct parser *ps);

struct module *
parse(struct parser *ps);

#endif
