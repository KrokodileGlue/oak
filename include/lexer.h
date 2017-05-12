#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "location.h"
#include "token.h"
#include "error.h"

struct LexState {
	char *text, *file;
	struct ErrorState *es;
	struct Token *tok;
	struct Location loc;
};

void lexer_clear(struct LexState *ls);
struct LexState *lexer_new(char *text, char *file);
struct Token *tokenize(struct LexState *ls);

#endif
