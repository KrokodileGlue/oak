#include "code.h"
#include "util.h"

struct instruction_data instruction_data[] = {
	{ INSTR_NOP,      REG_NONE,  "NOP       " },

	{ INSTR_MOV,      REG_AB,    "MOV       " },
	{ INSTR_COPY,     REG_AB,    "COPY      " },
	{ INSTR_COPYC,    REG_AB,    "COPYC     " },

	{ INSTR_JMP,      REG_A,     "JMP       " },
	{ INSTR_ESCAPE,   REG_A,     "ESCAPE    " },
	{ INSTR_PUSH,     REG_A,     "PUSH      " },
	{ INSTR_POP,      REG_A,     "POP       " },
	{ INSTR_POPALL,   REG_A,     "POPALL    " },
	{ INSTR_CALL,     REG_A,     "CALL      " },
	{ INSTR_RET,      REG_NONE,  "RET       " },
	{ INSTR_PUSHIMP,  REG_A,     "PUSHIMP   " },
	{ INSTR_POPIMP,   REG_NONE,  "POPIMP    " },
	{ INSTR_GETIMP,   REG_A,     "GETIMP    " },
	{ INSTR_CHKSTCK,  REG_NONE,  "CHKSTCK   " },

	{ INSTR_PUSHBACK, REG_AB,    "PUSHBACK  " },
	{ INSTR_ASET,     REG_ABC,   "ASET      " },
	{ INSTR_DEREF,    REG_ABC,   "DEREF     " },
	{ INSTR_SUBSCR,   REG_ABCD,  "SUBSCR    " },
	{ INSTR_SLICE,    REG_ABCDE, "SLICE     " },

	{ INSTR_MATCH,    REG_ABC,   "MATCH     " },
	{ INSTR_RESETR,   REG_A,     "RESETR    " },
	{ INSTR_SUBST,    REG_ABCD,  "SUBST     " },
	{ INSTR_GROUP,    REG_AB,    "GROUP     " },

	{ INSTR_MSET,     REG_A,     "MSET      " },
	{ INSTR_MINC,     REG_NONE,  "MINC      " },

	{ INSTR_SPLIT,    REG_ABC,   "SPLIT     " },
	{ INSTR_JOIN,     REG_ABC,   "JOIN      " },
	{ INSTR_RANGE,    REG_ABCD,  "RANGE     " },
	{ INSTR_PUSH,     REG_AB,    "APUSH     " },
	{ INSTR_REV,      REG_AB,    "REV       " },
	{ INSTR_SORT,     REG_AB,    "SORT      " },
	{ INSTR_SUM,      REG_AB,    "SUM       " },
	{ INSTR_ABS,      REG_AB,    "ABS       " },
	{ INSTR_COUNT,    REG_ABC,   "COUNT     " },

	{ INSTR_KEYS,     REG_AB,    "KEYS      " },
	{ INSTR_VALUES,   REG_AB,    "VALUES    " },

	{ INSTR_INT,      REG_AB,    "INT       " },
	{ INSTR_FLOAT,    REG_AB,    "FLOAT     " },
	{ INSTR_STR,      REG_AB,    "STR       " },

	{ INSTR_UC,       REG_AB,    "UC        " },
	{ INSTR_LC,       REG_AB,    "LC        " },
	{ INSTR_UCFIRST,  REG_AB,    "UCFIRST   " },
	{ INSTR_LCFIRST,  REG_AB,    "LCFIRST   " },

	{ INSTR_COND,     REG_A,     "COND      " },
	{ INSTR_NCOND,    REG_A,     "NCOND     " },
	{ INSTR_CMP,      REG_ABC,   "CMP       " },
	{ INSTR_LESS,     REG_ABC,   "LESS      " },
	{ INSTR_LEQ,      REG_ABC,   "LEQ       " },
	{ INSTR_GEQ,      REG_ABC,   "GEQ       " },
	{ INSTR_MORE,     REG_ABC,   "MORE      " },
	{ INSTR_INC,      REG_A,     "INC       " },
	{ INSTR_DEC,      REG_A,     "DEC       " },
	{ INSTR_TYPE,     REG_AB,    "TYPE      " },
	{ INSTR_LEN,      REG_AB,    "LEN       " },

	{ INSTR_MIN,      REG_A,     "MIN       " },
	{ INSTR_MAX,      REG_A,     "MAX       " },

	{ INSTR_ADD,      REG_ABC,   "ADD       " },
	{ INSTR_SUB,      REG_ABC,   "SUB       " },
	{ INSTR_MUL,      REG_ABC,   "MUL       " },
	{ INSTR_POW,      REG_ABC,   "POW       " },
	{ INSTR_DIV,      REG_ABC,   "DIV       " },

	{ INSTR_SLEFT,    REG_ABC,   "SLEFT     " },
	{ INSTR_SRIGHT,   REG_ABC,   "SRIGHT    " },

	{ INSTR_BAND,     REG_ABC,   "BAND      " },
	{ INSTR_XOR,      REG_ABC,   "XOR       " },
	{ INSTR_BOR,      REG_ABC,   "BOR       " },

	{ INSTR_MOD,      REG_ABC,   "MOD       " },
	{ INSTR_NEG,      REG_AB,    "NEG       " },
	{ INSTR_FLIP,     REG_AB,    "FLIP      " },

	{ INSTR_PRINT,    REG_A,     "PRINT     " },
	{ INSTR_LINE,     REG_NONE,  "LINE      " },

	{ INSTR_EVAL,     REG_ABC,   "EVAL      " },
	{ INSTR_KILL,     REG_A,     "KILL      " },
	{ INSTR_EEND,     REG_A,     "EEND      " },
	{ INSTR_END,      REG_A,     "END       " }
};

void
print_instruction(FILE *f, struct instruction c)
{
	fprintf(f, "%s ", instruction_data[c.type].name);
	switch (instruction_data[c.type].regtype) {
	case REG_A:
		fprintf(f, "%4d                        ", c.a);
		break;

	case REG_AB:
		fprintf(f, "%4d, %4d                  ", c.a, c.b);
		break;

	case REG_ABC:
		fprintf(f, "%4d, %4d, %4d            ", c.a, c.b, c.c);
		break;

	case REG_ABCD:
		fprintf(f, "%4d, %4d, %4d, %4d      ", c.a, c.b, c.c, c.d);
		break;

	case REG_ABCDE:
		fprintf(f, "%4d, %4d, %4d, %4d, %4d ", c.a, c.b, c.c, c.d, c.e);
		break;

	case REG_NONE:
		fprintf(f, "                            ");
		break;
	}

}

void
print_code(FILE *f, struct instruction *code, size_t num_instr)
{
	for (size_t i = 0; i < num_instr; i++) {
		fprintf(f, "\n%4zu: ", i);
		print_instruction(f, code[i]);
	}
}
