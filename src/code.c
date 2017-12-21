#include "code.h"
#include "util.h"

struct instruction_data instruction_data[] = {
	{ INSTR_MOVC,       "MOVC      " },
	{ INSTR_MOV,        "MOV       " },

	{ INSTR_ADD,        "ADD       " },
	{ INSTR_SUB,        "SUB       " },
	{ INSTR_MUL,        "MUL       " },
	{ INSTR_DIV,        "DIV       " },

	{ INSTR_PRINT,      "PRINT     " },
	{ INSTR_LINE,       "LINE      " },

	{ INSTR_END,        "END       " }
};

void
print_code(FILE *f, struct instruction *code, size_t num_instr)
{
	for (size_t i = 0; i < num_instr; i++) {
		fprintf(f, "\n%3zu: %s %d", i, instruction_data[code[i].type].name, code[i].d.a);
	}
}
