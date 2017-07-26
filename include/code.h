#ifndef CODE_H
#define CODE_H

#include <stdlib.h>
#include <stdio.h>

#include "location.h"

struct instruction {
	enum instruction_type {
		INSTR_PUSH_CONST,
		INSTR_POP_CONST,
		INSTR_PUSH_LOCAL,
		INSTR_POP_LOCAL,
		INSTR_POP,

		INSTR_ADD,
		INSTR_SUB,
		INSTR_MUL,
		INSTR_DIV,
		INSTR_INC,
		INSTR_DEC,

		INSTR_LESS,
		INSTR_MOD,

		INSTR_CMP,
		INSTR_AND,

		INSTR_COND_JUMP,
		INSTR_FALSE_JUMP,
		INSTR_JUMP,

		INSTR_PRINT,
		INSTR_LINE,

		INSTR_LIST,
		INSTR_SUBSCRIPT,

		INSTR_END
	} type;

	size_t arg;
	struct location loc;
};

struct instructionData {
	enum instruction_type  type;
	char                  *body;
};

extern struct instructionData instruction_data[];

void print_code(FILE *f, struct instruction *code, size_t num_instr);

#endif
