#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

#include "location.h"
#include "token.h"
#include "module.h"
#include "error.h"

struct lexer {
	char *text, *file;
	struct reporter   *r;
	struct token      *tok;
	struct location    loc;
};

void lexer_clear(struct lexer *ls);
struct lexer *new_lexer(char *text, char *file);
bool tokenize(struct module *m);

#endif
