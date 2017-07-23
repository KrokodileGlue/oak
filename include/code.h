#ifndef CODE_H
#define CODE_H

#include <stdlib.h>
#include <stdio.h>

struct instruction {
	enum instruction_type {
		INSTR_PUSH_CONST,
		INSTR_POP_CONST,
		INSTR_PUSH_LOCAL,
		INSTR_POP_LOCAL,

		INSTR_ADD,
		INSTR_SUB,
		INSTR_MUL,
		INSTR_DIV,

		INSTR_PRINT,
		INSTR_END
	} type;

	size_t a;
};

struct instructionData {
	enum instruction_type  type;
	char                  *body;
};

extern struct instructionData instruction_data[];

void print_code(FILE *f, struct instruction *code, size_t num_instr);

#endif
