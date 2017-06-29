#ifndef KEYWORD_H
#define KEYWORD_H

#include <stdlib.h>
#include <stdbool.h>

enum KeywordType {
	KEYWORD_FOR,
	KEYWORD_IF,
	KEYWORD_ELSE,
	KEYWORD_FN,
	KEYWORD_VAR,
	KEYWORD_PRINT,
	KEYWORD_INVALID
};

struct Keyword {
	enum KeywordType	 type;
	char			*body;
};

extern struct Keyword keywords[];
size_t num_keywords();

#endif
