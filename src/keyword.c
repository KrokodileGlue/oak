#include "keyword.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char keywords[][32] = {
	"for",
	"if",
	"fn",
	"var"
};

enum KeywordType keyword_get_type(char *str)
{
	for (size_t i = 0; i < sizeof keywords / sizeof *keywords; i++) {
		if (!strcmp(str, keywords[i])) return (enum KeywordType)i;
	}
	
	return KEYWORD_INVALID;
}
