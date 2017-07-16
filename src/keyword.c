#include "keyword.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct keyword keywords[] = {
	{ KEYWORD_FOR,     "for"     },
	{ KEYWORD_IF,      "if"      },
	{ KEYWORD_ELSE,    "else"    },
	{ KEYWORD_FN,      "fn"      },
	{ KEYWORD_VAR,     "var"     },
	{ KEYWORD_PRINT,   "print"   },
	{ KEYWORD_IN,      "in"      },
	{ KEYWORD_YIELD,   "yield"   },
	{ KEYWORD_WHILE,   "while"   },
	{ KEYWORD_DO,      "do"      },
	{ KEYWORD_CLASS,   "class"   },
	{ KEYWORD_IMPORT,  "import"  },
	{ KEYWORD_AS,      "as"      },
	{ KEYWORD_INVALID, "invalid" },
};

size_t
num_keywords()
{
	return sizeof keywords / sizeof *keywords;
}
