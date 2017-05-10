#ifndef KEYWORD_H
#define KEYWORD_H

#include <stdbool.h>

enum KeywordType {
	KEYWORD_FOR,
	KEYWORD_IF,
	KEYWORD_FN,
	KEYWORD_VAR,
	KEYWORD_INVALID
};

enum KeywordType keyword_get_type(char *str);

#endif
