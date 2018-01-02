#ifndef CODE_H
#define CODE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "location.h"

enum instruction_type {
	INSTR_MOVC,
	INSTR_MOV,

	INSTR_JMP,
	INSTR_PUSH,
	INSTR_POP,
	INSTR_CALL,
	INSTR_RET,

	INSTR_PUSHBACK,
	INSTR_ASET,
	INSTR_DEREF,

	INSTR_ADD,
	INSTR_SUB,
	INSTR_MUL,
	INSTR_DIV,

	INSTR_PRINT,
	INSTR_LINE,

	INSTR_END
};

struct instruction {
	int type;
	struct location *loc;

	union {
		uint8_t a;

		struct {
			uint8_t b;
			uint8_t c;
		} bc;

		uint16_t d;

		struct {
			uint8_t e;
			uint8_t f;
			uint8_t g;
		} efg;
	} d;
};

struct instruction_data {
	enum instruction_type type;
	enum {
		REG_NONE,
		REG_A,
		REG_BC,
		REG_D,
		REG_EFG,
	} regtype;
	char *name;
};

extern struct instruction_data instruction_data[];
void print_instruction(FILE *f, struct instruction c);
void print_code(FILE *f, struct instruction *code, size_t num_instr);

#endif
