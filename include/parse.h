#ifndef PARSE_H
#define PARSE_H

#include "tree.h"
#include "token.h"
#include "error.h"
#include "module.h"

struct parser {
	struct reporter *r;
	struct token *tok;
};

struct parser *new_parser();
void parser_clear(struct parser *ps);
bool parse(struct module *m);

#endif
