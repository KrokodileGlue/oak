#ifndef KEYWORD_H
#define KEYWORD_H

#include <stdlib.h>
#include <stdbool.h>

enum keyword_type {
	KEYWORD_FOR,
	KEYWORD_IF,
	KEYWORD_ELSE,
	KEYWORD_FN,
	KEYWORD_VAR,
	KEYWORD_PRINT,
	KEYWORD_PRINTLN,
	KEYWORD_IN,
	KEYWORD_RET,
	KEYWORD_WHILE,
	KEYWORD_DO,
	KEYWORD_CLASS,
	KEYWORD_IMPORT,
	KEYWORD_AS,
	KEYWORD_INVALID
};

struct keyword {
	enum keyword_type	 type;
	char			*body;
};

extern struct keyword keywords[];

size_t
num_keywords();

#endif
