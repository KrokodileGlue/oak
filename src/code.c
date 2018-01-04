#include "code.h"
#include "util.h"

struct instruction_data instruction_data[] = {
	{ INSTR_MOVC,     REG_BC,   "MOVC      " },
	{ INSTR_MOV,      REG_BC,   "MOV       " },
	{ INSTR_GMOV,     REG_BC,   "GMOV      " },
	{ INSTR_MOVG,     REG_BC,   "MOVG      " },

	{ INSTR_JMP,      REG_D,    "JMP       " },
	{ INSTR_PUSH,     REG_A,    "PUSH      " },
	{ INSTR_POP,      REG_A,    "POP       " },
	{ INSTR_CALL,     REG_D,    "CALL      " },
	{ INSTR_RET,      REG_NONE, "RET       " },

	{ INSTR_PUSHBACK, REG_BC,   "PUSHBACK  " },
	{ INSTR_ASET,     REG_EFG,  "ASET      " },
	{ INSTR_DEREF,    REG_EFG,  "DEREF     " },
	{ INSTR_SUBSCR,   REG_EFG,  "SUBSCR    " },

	{ INSTR_COND,     REG_A,    "COND      " },
	{ INSTR_NCOND,    REG_A,    "NCOND     " },
	{ INSTR_CMP,      REG_EFG,  "CMP       " },
	{ INSTR_LESS,     REG_EFG,  "LESS      " },
	{ INSTR_INC,      REG_A,    "INC       " },
	{ INSTR_GINC,     REG_A,    "GINC      " },
	{ INSTR_TYPE,     REG_BC,   "TYPE      " },

	{ INSTR_ADD,      REG_EFG,  "ADD       " },
	{ INSTR_SUB,      REG_EFG,  "SUB       " },
	{ INSTR_MUL,      REG_EFG,  "MUL       " },
	{ INSTR_DIV,      REG_EFG,  "DIV       " },
	{ INSTR_MOD,      REG_EFG,  "MOD       " },
	{ INSTR_OR,       REG_EFG,  "OR        " },

	{ INSTR_PRINT,    REG_A,    "PRINT     " },
	{ INSTR_LINE,     REG_NONE, "LINE      " },

	{ INSTR_END,      REG_NONE, "END       " }
};

void
print_instruction(FILE *f, struct instruction c)
{
	fprintf(f, "%s", instruction_data[c.type].name);
	switch (instruction_data[c.type].regtype) {
	case REG_A: fprintf(f, " %d", c.d.a); break;
	case REG_BC:
		fprintf(f, " %d, %d", c.d.bc.b, c.d.bc.c);
		break;
	case REG_D: fprintf(f, " %d", c.d.d); break;
	case REG_EFG:
		fprintf(f, " %d, %d, %d", c.d.efg.e,
		        c.d.efg.f, c.d.efg.g);
		break;
	case REG_NONE: break;
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
