#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "location.h"
#include "token.h"
#include "error.h"

struct lexer {
	char *text, *file;
	struct error_state *es;
	struct token       *tok;
	struct location    loc;
};

void
lexer_clear(struct lexer *ls);

struct lexer *
lexer_new(char *text, char *file);

struct token *
tokenize(struct lexer *ls);

#endif
