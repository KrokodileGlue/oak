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
	{ "==",  "",  9,  ASS_LEFT,  OPTYPE_BINARY   , OP_CMP         },
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
	{ ".=",  "",  2,  ASS_RIGHT, OPTYPE_BINARY   , OP_DOTEQ       },
	{ "-=",  "",  2,  ASS_RIGHT, OPTYPE_BINARY   , OP_SUBEQ       },
	{ "*=",  "",  2,  ASS_RIGHT, OPTYPE_BINARY   , OP_MULEQ       },
	{ "/=",  "",  2,  ASS_RIGHT, OPTYPE_BINARY   , OP_DIVEQ       },
	{ ",",   "",  1,  ASS_RIGHT, OPTYPE_BINARY   , OP_COMMA       }
};

/*
 * TODO: merge instructions for binary operators into a single
 * INSTR_BINOP or something.
 */
struct operator_instr binop_instr[] = {
	{ OP_ADD, INSTR_ADD },
	{ OP_SUB, INSTR_SUB },
	{ OP_MUL, INSTR_MUL },
	{ OP_DIV, INSTR_DIV },
	{ OP_POW, INSTR_POW },
	{ OP_MOD, INSTR_MOD },

	{ OP_LESS, INSTR_LESS },
	{ OP_MORE, INSTR_MORE },
	{ OP_LEQ, INSTR_LEQ },
	{ OP_GEQ, INSTR_GEQ },
	{ OP_CMP, INSTR_CMP },

	{ OP_XOR, INSTR_XOR },
	{ OP_BOR, INSTR_BOR },
	{ OP_BAND, INSTR_BAND },

	{ OP_LEFT, INSTR_SLEFT },
	{ OP_RIGHT, INSTR_SRIGHT },

	/* Invalid in array mutators */
	{ OP_NOTEQ, INSTR_NOP }, /* TODO: this should be allowed */
	{ OP_IFF, INSTR_NOP },
	{ OP_COMMA, INSTR_NOP },
	{ OP_MODMOD, INSTR_NOP },
	{ OP_CC, INSTR_NOP }
};

size_t
num_ops()
{
	return sizeof ops / sizeof *ops;
}

size_t num_binop_instr()
{
	return sizeof binop_instr / sizeof *binop_instr;
}
