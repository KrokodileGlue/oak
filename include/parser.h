#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "token.h"
#include "error.h"

struct ParseState {
	struct ErrorState *es;
	struct Token *tok;
};

struct ParseState *parser_new();
void parser_clear(struct ParseState *ps);

struct Statement **parse(struct ParseState *ps);

#endif
