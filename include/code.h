#ifndef CODE_H
#define CODE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "location.h"

#define NUM_REG (1 << 10)

enum instruction_type {
	INSTR_NOP,

	INSTR_MOV,
	INSTR_COPY,
	INSTR_COPYC,
	INSTR_MOVC,

	INSTR_JMP,
	INSTR_ESCAPE,
	INSTR_PUSH,
	INSTR_POP,
	INSTR_POPALL,
	INSTR_CALL,
	INSTR_RET,
	INSTR_PUSHIMP,
	INSTR_POPIMP,
	INSTR_GETIMP,
	INSTR_CHKSTCK,

	INSTR_PUSHBACK,
	INSTR_ASET,
	INSTR_DEREF,
	INSTR_SUBSCR,
	INSTR_SLICE,

	INSTR_MATCH,
	INSTR_RESETR,
	INSTR_SUBST,
	INSTR_GROUP,

	INSTR_MSET,
	INSTR_MINC,

	INSTR_SPLIT,
	INSTR_JOIN,
	INSTR_RANGE,
	INSTR_APUSH,
	INSTR_APOP,
	INSTR_SHIFT,
	INSTR_INS,
	INSTR_REV,
	INSTR_SORT,
	INSTR_SUM,
	INSTR_ABS,
	INSTR_COUNT,

	INSTR_KEYS,
	INSTR_VALUES,

	INSTR_INT,
	INSTR_FLOAT,
	INSTR_STR,

	INSTR_UC,
	INSTR_LC,
	INSTR_UCFIRST,
	INSTR_LCFIRST,

	INSTR_COND,
	INSTR_NCOND,
	INSTR_CMP,
	INSTR_LESS,
	INSTR_LEQ,
	INSTR_GEQ,
	INSTR_MORE,
	INSTR_INC,
	INSTR_DEC,
	INSTR_TYPE,
	INSTR_LEN,

	INSTR_MIN,
	INSTR_MAX,

	INSTR_CHR,
	INSTR_ORD,
	INSTR_RJUST,

	INSTR_HEX,
	INSTR_CHOMP,
	INSTR_TRIM,

	INSTR_LASTOF,

	INSTR_ADD,
	INSTR_SUB,
	INSTR_MUL,
	INSTR_POW,
	INSTR_DIV,

	INSTR_SLEFT,
	INSTR_SRIGHT,

	INSTR_BAND,
	INSTR_XOR,
	INSTR_BOR,

	INSTR_MOD,
	INSTR_NEG,
	INSTR_FLIP,

	INSTR_PRINT,
	INSTR_LINE,

	INSTR_EVAL,
	INSTR_INTERP,
	INSTR_KILL,
	INSTR_EEND,
	INSTR_END
};

struct instruction {
	unsigned char type;
	struct location *loc;

	uint16_t a;
	uint16_t b;
	uint16_t c;
	uint16_t d;
	uint16_t e;
};

struct instruction_data {
	enum instruction_type type;
	enum {
		REG_NONE,
		REG_A,
		REG_AB,
		REG_ABC,
		REG_ABCD,
		REG_ABCDE
	} regtype;
	char *name;
};

extern struct instruction_data instruction_data[];
void print_instruction(FILE *f, struct instruction c);
void print_code(FILE *f, struct instruction *code, size_t num_instr);

#endif
