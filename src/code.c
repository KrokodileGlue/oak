#include "code.h"
#include "util.h"

struct instruction_data instruction_data[] = {
	{ INSTR_MOVC,  REG_BC,   "MOVC      " },
	{ INSTR_MOV,   REG_BC,   "MOV       " },

	{ INSTR_JMP,   REG_A,    "JMP       " },
	{ INSTR_PUSH,  REG_A,    "PUSH      " },
	{ INSTR_POP,   REG_A,    "POP       " },
	{ INSTR_CALL,  REG_A,    "CALL      " },
	{ INSTR_RET,   REG_NONE, "RET       " },

	{ INSTR_ADD,   REG_EFG,  "ADD       " },
	{ INSTR_SUB,   REG_EFG,  "SUB       " },
	{ INSTR_MUL,   REG_EFG,  "MUL       " },
	{ INSTR_DIV,   REG_EFG,  "DIV       " },

	{ INSTR_PRINT, REG_A,    "PRINT     " },
	{ INSTR_LINE,  REG_NONE, "LINE      " },

	{ INSTR_END,   REG_NONE, "END       " }
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
