#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "location.h"
#include "token.h"

struct Token* tokenize(char* code, char* filename);

#endif
