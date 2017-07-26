#include "code.h"
#include "util.h"

struct instructionData instruction_data[] = {
	{ INSTR_PUSH_CONST, "PUSH_CONST" },
	{ INSTR_POP_CONST,  "POP_CONST " },
	{ INSTR_PUSH_LOCAL, "PUSH_LOCAL" },
	{ INSTR_POP_LOCAL,  "POP_LOCAL " },
	{ INSTR_POP,        "POP       " },

	{ INSTR_ADD,        "ADD       " },
	{ INSTR_SUB,        "SUB       " },
	{ INSTR_MUL,        "MUL       " },
	{ INSTR_DIV,        "DIV       " },
	{ INSTR_INC,        "INC       " },
	{ INSTR_DEC,        "DEC       " },

	{ INSTR_LESS,       "LESS      " },
	{ INSTR_MOD,        "MOD       " },

	{ INSTR_CMP,        "CMP       "},
	{ INSTR_AND,        "AND       "},

	{ INSTR_COND_JUMP,  "COND_JUMP "},
	{ INSTR_FALSE_JUMP, "FALSE_JUMP"},
	{ INSTR_JUMP,       "JUMP      "},

	{ INSTR_PRINT,      "PRINT     " },
	{ INSTR_LINE,       "LINE " },

	{ INSTR_LIST,       "LIST      " },
	{ INSTR_SUBSCRIPT,  "SUBSCRIPT " },

	{ INSTR_END,        "END       " }
};

void
print_code(FILE *f, struct instruction *code, size_t num_instr)
{
	for (size_t i = 0; i < num_instr; i++) {
		fprintf(f, "\n%3zu: %s %zu", i, instruction_data[code[i].type].body, code[i].arg);
	}
}
