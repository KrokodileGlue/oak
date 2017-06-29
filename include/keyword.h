#ifndef KEYWORD_H
#define KEYWORD_H

#include <stdbool.h>

enum KeywordType {
	KEYWORD_FOR,
	KEYWORD_IF,
	KEYWORD_FN,
	KEYWORD_VAR,
	KEYWORD_PRINT,
	KEYWORD_INVALID
};

struct Keyword {
	enum KeywordType type;
	char *value;
};

enum KeywordType keyword_get_type(char *str);
char *keyword_get_type_str(enum KeywordType type);

#endif
