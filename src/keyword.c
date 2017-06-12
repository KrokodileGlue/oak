#include "keyword.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Keyword keywords[] = {
	{ KEYWORD_FOR, "for" },
	{ KEYWORD_IF,  "if"  },
	{ KEYWORD_FN,  "fn"  },
	{ KEYWORD_VAR, "var" }
};

enum KeywordType keyword_get_type(char *str)
{
	for (size_t i = 0; i < sizeof keywords / sizeof *keywords; i++) {
		if (!strcmp(str, keywords[i].value)) return keywords[i].type;
	}
	return KEYWORD_INVALID;
}

char *keyword_get_type_str(enum KeywordType type)
{
	return keywords[type].value;
}
