#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "token.h"
#include "error.h"

struct Statement **parse(struct ErrorState *es, struct Token *tok);

#endif
