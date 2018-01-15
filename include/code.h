#ifndef CODE_H
#define CODE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "location.h"

enum instruction_type {
	INSTR_NOP,

	INSTR_MOVC,
	INSTR_MOV,

	INSTR_COPY,
	INSTR_COPYC,

	INSTR_JMP,
	INSTR_ESCAPE,
	INSTR_PUSH,
	INSTR_POP,
	INSTR_CALL,
	INSTR_RET,
	INSTR_PUSHIMP,
	INSTR_POPIMP,
	INSTR_GETIMP,

	INSTR_PUSHBACK,
	INSTR_ASET,
	INSTR_DEREF,
	INSTR_SUBSCR,

	INSTR_MATCH,
	INSTR_SUBST,
	INSTR_SPLIT,
	INSTR_GROUP,

	INSTR_COND,
	INSTR_NCOND,
	INSTR_CMP,
	INSTR_LESS,
	INSTR_LEQ,
	INSTR_MORE,
	INSTR_INC,
	INSTR_TYPE,
	INSTR_LEN,

	INSTR_ADD,
	INSTR_SUB,
	INSTR_MUL,
	INSTR_POW,
	INSTR_DIV,
	INSTR_MOD,
	INSTR_OR,
	INSTR_NEG,
	INSTR_FLIP,

	INSTR_PRINT,
	INSTR_LINE,

	INSTR_EVAL,
	INSTR_KILL,
	INSTR_END
};

struct instruction {
	int type;
	struct location *loc;

	union {
		uint16_t a;

		struct {
			uint16_t b;
			uint16_t c;
		} bc;

		uint16_t d;

		struct {
			uint16_t e;
			uint16_t f;
			uint16_t g;
			uint16_t h;
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
