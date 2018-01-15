#include "code.h"
#include "util.h"

struct instruction_data instruction_data[] = {
	{ INSTR_NOP,      REG_NONE, "NOP       " },

	{ INSTR_MOVC,     REG_BC,   "MOVC      " },
	{ INSTR_MOV,      REG_BC,   "MOV       " },

	{ INSTR_COPY,     REG_BC,   "COPY      " },
	{ INSTR_COPYC,    REG_BC,   "COPYC     " },

	{ INSTR_JMP,      REG_D,    "JMP       " },
	{ INSTR_ESCAPE,   REG_D,    "ESCAPE    " },
	{ INSTR_PUSH,     REG_A,    "PUSH      " },
	{ INSTR_POP,      REG_A,    "POP       " },
	{ INSTR_CALL,     REG_A,    "CALL      " },
	{ INSTR_RET,      REG_NONE, "RET       " },
	{ INSTR_PUSHIMP,  REG_A,    "PUSHIMP   " },
	{ INSTR_POPIMP,   REG_A,    "POPIMP    " },
	{ INSTR_GETIMP,   REG_A,    "GETIMP    " },

	{ INSTR_PUSHBACK, REG_BC,   "PUSHBACK  " },
	{ INSTR_ASET,     REG_EFG,  "ASET      " },
	{ INSTR_DEREF,    REG_EFG,  "DEREF     " },
	{ INSTR_SUBSCR,   REG_EFG,  "SUBSCR    " },

	{ INSTR_MATCH,    REG_EFG,  "MATCH     " },
	{ INSTR_SUBST,    REG_EFG,  "SUBST     " },
	{ INSTR_SPLIT,    REG_EFG,  "SPLIT     " },
	{ INSTR_GROUP,    REG_BC,   "GROUP     " },

	{ INSTR_COND,     REG_A,    "COND      " },
	{ INSTR_NCOND,    REG_A,    "NCOND     " },
	{ INSTR_CMP,      REG_EFG,  "CMP       " },
	{ INSTR_LESS,     REG_EFG,  "LESS      " },
	{ INSTR_MORE,     REG_EFG,  "MORE      " },
	{ INSTR_INC,      REG_A,    "INC       " },
	{ INSTR_TYPE,     REG_BC,   "TYPE      " },
	{ INSTR_LEN,      REG_BC,   "LEN       " },

	{ INSTR_ADD,      REG_EFG,  "ADD       " },
	{ INSTR_SUB,      REG_EFG,  "SUB       " },
	{ INSTR_MUL,      REG_EFG,  "MUL       " },
	{ INSTR_POW,      REG_EFG,  "POW       " },
	{ INSTR_DIV,      REG_EFG,  "DIV       " },
	{ INSTR_MOD,      REG_EFG,  "MOD       " },
	{ INSTR_OR,       REG_EFG,  "OR        " },
	{ INSTR_NEG,      REG_BC,   "NEG       " },
	{ INSTR_FLIP,     REG_BC,   "FLIP      " },

	{ INSTR_PRINT,    REG_A,    "PRINT     " },
	{ INSTR_LINE,     REG_NONE, "LINE      " },

	{ INSTR_EVAL,     REG_EFG,  "EVAL      " },
	{ INSTR_KILL,     REG_A,    "KILL      " },
	{ INSTR_END,      REG_A,    "END       " }
};

void
print_instruction(FILE *f, struct instruction c)
{
	fprintf(f, "%s", instruction_data[c.type].name);
	switch (instruction_data[c.type].regtype) {
	case REG_A: fprintf(f, " %3d          ", c.d.a); break;
	case REG_BC:
		fprintf(f, " %3d, %3d     ", c.d.bc.b, c.d.bc.c);
		break;
	case REG_D: fprintf(f, " %3d          ", c.d.d); break;
	case REG_EFG:
		fprintf(f, " %3d, %3d, %3d", c.d.efg.e,
		        c.d.efg.f, c.d.efg.g);
		break;
	case REG_NONE: fprintf(f, "              "); break;
	}

}

void
print_code(FILE *f, struct instruction *code, size_t num_instr)
{
	for (size_t i = 0; i < num_instr; i++) {
		fprintf(f, "\n%3zu: ", i);
		print_instruction(f, code[i]);
	}
}
