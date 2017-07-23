#include "code.h"
#include "util.h"

struct instructionData instruction_data[] = {
	{ INSTR_PUSH_CONST, "PUSH_CONST" },
	{ INSTR_POP_CONST,  "POP_CONST " },
	{ INSTR_PUSH_LOCAL, "PUSH_LOCAL" },
	{ INSTR_POP_LOCAL,  "POP_LOCAL " },

	{ INSTR_ADD,        "ADD       " },
	{ INSTR_SUB,        "SUB       " },
	{ INSTR_MUL,        "MUL       " },
	{ INSTR_DIV,        "DIV       " },

	{ INSTR_PRINT,      "PRINT     " },

	{ INSTR_END,        "END       " },
};

void
print_code(FILE *f, struct instruction *code, size_t num_instr)
{
	for (size_t i = 0; i < num_instr; i++) {
		fprintf(f, "\n%s %zu", instruction_data[code[i].type].body, code[i].a);
	}
}
