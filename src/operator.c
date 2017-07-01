#include "operator.h"

struct Operator ops[] = {
	{ "(",   ")", 15, ASS_LEFT,  OP_MEMBER   },
	{ "[",   "]", 15, ASS_LEFT,  OP_MEMBER   },

	{ "++",  "",  15, ASS_LEFT,  OP_POSTFIX  },
	{ "--",  "",  15, ASS_LEFT,  OP_POSTFIX  },

	{ "+",   "",  14, ASS_RIGHT, OP_PREFIX   },
	{ "-",   "",  14, ASS_RIGHT, OP_PREFIX   },
	{ "type","",  14, ASS_RIGHT, OP_PREFIX   },
	{ "*",   "",  13, ASS_LEFT,  OP_BINARY   },
	{ "/",   "",  13, ASS_LEFT,  OP_BINARY   },
	{ "%",   "",  13, ASS_LEFT,  OP_BINARY   },
	{ "+",   "",  12, ASS_LEFT,  OP_BINARY   },
	{ "-",   "",  12, ASS_LEFT,  OP_BINARY   },
	{ "->",  "",  12, ASS_LEFT,  OP_BINARY   },

	{ "<",   "",  10, ASS_LEFT,  OP_BINARY   },
	{ ">",   "",  10, ASS_LEFT,  OP_BINARY   },
	{ "<=",  "",  10, ASS_LEFT,  OP_BINARY   },
	{ ">=",  "",  10, ASS_LEFT,  OP_BINARY   },

	{ "==",  "",  9,  ASS_LEFT,  OP_BINARY   },
	{ "&&",  "",  5,  ASS_LEFT,  OP_BINARY   },
	{ "and", "",  5,  ASS_LEFT,  OP_BINARY   },
	{ "||",  "",  4,  ASS_LEFT,  OP_BINARY   },
	{ "or",  "",  4,  ASS_LEFT,  OP_BINARY   },
	{ "?",   ":", 3,  ASS_LEFT,  OP_TERNARY  },
	{ "=",   "",  2,  ASS_LEFT,  OP_BINARY   },
	{ ",",   "",  1,  ASS_RIGHT, OP_BINARY   }
};

size_t num_ops()
{
	return sizeof ops / sizeof *ops;
}
