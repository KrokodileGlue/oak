#include "code.h"
#include "util.h"

struct instruction_data instruction_data[] = {
	{ INSTR_NOP,      REG_NONE, "NOP       " },

	{ INSTR_MOV,      REG_BC,   "MOV       " },
	{ INSTR_COPY,     REG_BC,   "COPY      " },
	{ INSTR_COPYC,    REG_BC,   "COPYC     " },

	{ INSTR_JMP,      REG_D,    "JMP       " },
	{ INSTR_ESCAPE,   REG_D,    "ESCAPE    " },
	{ INSTR_PUSH,     REG_A,    "PUSH      " },
	{ INSTR_POP,      REG_A,    "POP       " },
	{ INSTR_POPALL,   REG_A,    "POPALL    " },
	{ INSTR_CALL,     REG_A,    "CALL      " },
	{ INSTR_RET,      REG_NONE, "RET       " },
	{ INSTR_PUSHIMP,  REG_A,    "PUSHIMP   " },
	{ INSTR_POPIMP,   REG_NONE, "POPIMP    " },
	{ INSTR_GETIMP,   REG_A,    "GETIMP    " },
	{ INSTR_CHKSTCK,  REG_NONE, "CHKSTCK   " },

	{ INSTR_PUSHBACK, REG_BC,   "PUSHBACK  " },
	{ INSTR_ASET,     REG_EFG,  "ASET      " },
	{ INSTR_DEREF,    REG_EFG,  "DEREF     " },
	{ INSTR_SUBSCR,   REG_EFG,  "SUBSCR    " },

	{ INSTR_MATCH,    REG_EFG,  "MATCH     " },
	{ INSTR_SUBST,    REG_EFG,  "SUBST     " },
	{ INSTR_GROUP,    REG_BC,   "GROUP     " },

	{ INSTR_SPLIT,    REG_EFG,  "SPLIT     " },
	{ INSTR_JOIN,     REG_EFG,  "JOIN      " },
	{ INSTR_RANGE,    REG_EFG,  "RANGE     " },
	{ INSTR_PUSH,     REG_BC,   "APUSH     " },
	{ INSTR_REV,      REG_BC,   "REV       " },
	{ INSTR_SORT,     REG_BC,   "SORT      " },
	{ INSTR_SUM,      REG_BC,   "SUM       " },
	{ INSTR_ABS,      REG_BC,   "ABS       " },
	{ INSTR_COUNT,    REG_EFG,  "COUNT     " },

	{ INSTR_KEYS,     REG_BC,   "KEYS      " },
	{ INSTR_VALUES,   REG_BC,   "VALUES    " },

	{ INSTR_INT,      REG_BC,   "INT       " },
	{ INSTR_FLOAT,    REG_BC,   "FLOAT     " },
	{ INSTR_STR,      REG_BC,   "STR       " },

	{ INSTR_UC,       REG_BC,   "UC        " },
	{ INSTR_LC,       REG_BC,   "LC        " },
	{ INSTR_UCFIRST,  REG_BC,   "UCFIRST   " },
	{ INSTR_LCFIRST,  REG_BC,   "LCFIRST   " },

	{ INSTR_COND,     REG_A,    "COND      " },
	{ INSTR_NCOND,    REG_A,    "NCOND     " },
	{ INSTR_CMP,      REG_EFG,  "CMP       " },
	{ INSTR_LESS,     REG_EFG,  "LESS      " },
	{ INSTR_LEQ,      REG_EFG,  "LEQ       " },
	{ INSTR_GEQ,      REG_EFG,  "GEQ       " },
	{ INSTR_MORE,     REG_EFG,  "MORE      " },
	{ INSTR_INC,      REG_A,    "INC       " },
	{ INSTR_DEC,      REG_A,    "DEC       " },
	{ INSTR_TYPE,     REG_BC,   "TYPE      " },
	{ INSTR_LEN,      REG_BC,   "LEN       " },

	{ INSTR_MIN,      REG_A,    "MIN       " },
	{ INSTR_MAX,      REG_A,    "MAX       " },

	{ INSTR_ADD,      REG_EFG,  "ADD       " },
	{ INSTR_SUB,      REG_EFG,  "SUB       " },
	{ INSTR_MUL,      REG_EFG,  "MUL       " },
	{ INSTR_POW,      REG_EFG,  "POW       " },
	{ INSTR_DIV,      REG_EFG,  "DIV       " },

	{ INSTR_SLEFT,    REG_EFG,  "SLEFT     " },
	{ INSTR_SRIGHT,   REG_EFG,  "SRIGHT    " },

	{ INSTR_BAND,     REG_EFG,  "BAND      " },
	{ INSTR_XOR,      REG_EFG,  "XOR       " },
	{ INSTR_BOR,      REG_EFG,  "BOR       " },

	{ INSTR_MOD,      REG_EFG,  "MOD       " },
	{ INSTR_NEG,      REG_BC,   "NEG       " },
	{ INSTR_FLIP,     REG_BC,   "FLIP      " },

	{ INSTR_PRINT,    REG_A,    "PRINT     " },
	{ INSTR_LINE,     REG_NONE, "LINE      " },

	{ INSTR_EVAL,     REG_EFG,  "EVAL      " },
	{ INSTR_KILL,     REG_A,    "KILL      " },
	{ INSTR_EEND,     REG_A,    "EEND      " },
	{ INSTR_END,      REG_A,    "END       " }
};

void
print_instruction(FILE *f, struct instruction c)
{
	fprintf(f, "%s ", instruction_data[c.type].name);
	switch (instruction_data[c.type].regtype) {
	case REG_A: fprintf(f, "%4d             ", c.d.a); break;
	case REG_BC:
		fprintf(f,     "%4d, %4d       ", c.d.bc.b, c.d.bc.c);
		break;
	case REG_D: fprintf(f, "%4d             ", c.d.d); break;
	case REG_EFG:
		fprintf(f,     "%4d, %4d, %4d ", c.d.efg.e,
		        c.d.efg.f, c.d.efg.g);
		break;
	case REG_NONE: fprintf(f, "                 "); break;
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
