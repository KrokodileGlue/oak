#include "operator.h"

struct operator ops[] = {
	{ "(",   ")", 15, ASS_LEFT,  OPTYPE_FN_CALL  , OP_LBRACK      },
	{ "::",  "",  15, ASS_LEFT,  OPTYPE_BINARY   , OP_CC          },
	{ ".",   "",  15, ASS_LEFT,  OPTYPE_BINARY   , OP_PERIOD      },
	{ "[",   "]", 15, ASS_LEFT,  OPTYPE_SUBSCRIPT, OP_LBRACK      },

	{ "++",  "",  15, ASS_LEFT,  OPTYPE_POSTFIX  , OP_ADDADD      },
	{ "--",  "",  15, ASS_LEFT,  OPTYPE_POSTFIX  , OP_SUBSUB      },

	{ "++",  "",  14, ASS_RIGHT, OPTYPE_PREFIX   , OP_ADDADD      },
	{ "--",  "",  14, ASS_RIGHT, OPTYPE_PREFIX   , OP_SUBSUB      },
	{ "!",   "",  14, ASS_RIGHT, OPTYPE_PREFIX   , OP_EXCLAMATION },
	{ "not", "",  14, ASS_RIGHT, OPTYPE_PREFIX   , OP_EXCLAMATION },

	{ "+",   "",  14, ASS_RIGHT, OPTYPE_PREFIX   , OP_ADD         },
	{ "-",   "",  14, ASS_RIGHT, OPTYPE_PREFIX   , OP_SUB         },

	{ "*",   "",  13, ASS_LEFT,  OPTYPE_BINARY   , OP_MUL         },
	{ "**",  "",  13, ASS_LEFT,  OPTYPE_BINARY   , OP_POW         },
	{ "/",   "",  13, ASS_LEFT,  OPTYPE_BINARY   , OP_DIV         },
	{ "%",   "",  13, ASS_LEFT,  OPTYPE_BINARY   , OP_MOD         },
	{ "%%",  "",  13, ASS_LEFT,  OPTYPE_BINARY   , OP_MODMOD      },
	{ "+",   "",  12, ASS_LEFT,  OPTYPE_BINARY   , OP_ADD         },
	{ "-",   "",  12, ASS_LEFT,  OPTYPE_BINARY   , OP_SUB         },

	{ "&",   "",  8,  ASS_LEFT,  OPTYPE_BINARY   , OP_BAND        },
	{ "^",   "",  7,  ASS_LEFT,  OPTYPE_BINARY   , OP_XOR         },
	{ "|",   "",  6,  ASS_LEFT,  OPTYPE_BINARY   , OP_BOR         },

	{ "<<",  "",  5,  ASS_LEFT,  OPTYPE_BINARY   , OP_LEFT        },
	{ ">>",  "",  5,  ASS_LEFT,  OPTYPE_BINARY   , OP_RIGHT       },

	{ "<",   "",  10, ASS_LEFT,  OPTYPE_BINARY   , OP_LESS        },
	{ ">",   "",  10, ASS_LEFT,  OPTYPE_BINARY   , OP_MORE        },
	{ "<=",  "",  10, ASS_LEFT,  OPTYPE_BINARY   , OP_LEQ         },
	{ ">=",  "",  10, ASS_LEFT,  OPTYPE_BINARY   , OP_GEQ         },
	{ "==",  "",  9,  ASS_LEFT,  OPTYPE_BINARY   , OP_EQEQ        },
	{ "!=",  "",  9,  ASS_LEFT,  OPTYPE_BINARY   , OP_NOTEQ       },
	{ "iff", "",  8,  ASS_LEFT,  OPTYPE_BINARY   , OP_IFF         },

	{ "&&",  "",  5,  ASS_LEFT,  OPTYPE_BINARY   , OP_AND         },
	{ "and", "",  5,  ASS_LEFT,  OPTYPE_BINARY   , OP_AND         },
	{ "||",  "",  4,  ASS_LEFT,  OPTYPE_BINARY   , OP_OR          },
	{ "or",  "",  4,  ASS_LEFT,  OPTYPE_BINARY   , OP_OR          },
	{ "nor", "",  4,  ASS_LEFT,  OPTYPE_BINARY   , OP_NOR         },
	{ "?",   ":", 3,  ASS_LEFT,  OPTYPE_TERNARY  , OP_QUESTION    },
	{ "=",   "",  2,  ASS_RIGHT, OPTYPE_BINARY   , OP_EQ          },
	{ "->",  "",  2,  ASS_LEFT,  OPTYPE_BINARY   , OP_ARROW       },
	{ "=~",  "",  2,  ASS_RIGHT, OPTYPE_BINARY   , OP_SQUIGGLEEQ  },
	{ "+=",  "",  2,  ASS_RIGHT, OPTYPE_BINARY   , OP_ADDEQ       },
	{ "-=",  "",  2,  ASS_RIGHT, OPTYPE_BINARY   , OP_SUBEQ       },
	{ ",",   "",  1,  ASS_RIGHT, OPTYPE_BINARY   , OP_COMMA       }
};

size_t
num_ops()
{
	return sizeof ops / sizeof *ops;
}
