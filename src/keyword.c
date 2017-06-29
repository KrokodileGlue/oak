#include "keyword.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Keyword keywords[] = {
	{ KEYWORD_FOR,   "for"   },
	{ KEYWORD_IF,    "if"    },
	{ KEYWORD_ELSE,  "else"  },
	{ KEYWORD_FN,    "fn"    },
	{ KEYWORD_VAR,   "var"   },
	{ KEYWORD_PRINT, "print" }
};

size_t num_keywords()
{
	return sizeof keywords / sizeof *keywords;
}
