#ifndef CODE_H
#define CODE_H

#include <stdlib.h>
#include <stdio.h>

#include "location.h"

struct instruction {
	enum instruction_type {
		INSTR_PUSH_CONST,
		INSTR_PUSH_LOCAL,
		INSTR_POP_LOCAL,
		INSTR_POP,

		INSTR_ADD,
		INSTR_SUB,
		INSTR_MUL,
		INSTR_DIV,

		INSTR_INC,
		INSTR_DEC,
		INSTR_NEG,
		INSTR_ASSIGN,

		INSTR_LESS,
		INSTR_MORE,
		INSTR_MOD,

		INSTR_CMP,
		INSTR_FLIP,
		INSTR_AND,
		INSTR_OR,
		INSTR_LEN,
		INSTR_SAY,
		INSTR_TYPE,

		INSTR_COND_JUMP,
		INSTR_FALSE_JUMP,
		INSTR_TRUE_JUMP,
		INSTR_JUMP,
		INSTR_FRAME,
		INSTR_POP_FRAME,

		INSTR_CALL,
		INSTR_RET,

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
