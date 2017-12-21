#ifndef CODE_H
#define CODE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "location.h"

enum instruction_type {
	INSTR_MOVC,
	INSTR_MOV,

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
	uint16_t module;

	union {
		uint8_t a;
		struct {
			uint8_t b;
			uint8_t c;
		} bc;
		uint16_t d;
	} d;
};

struct instruction_data {
	enum instruction_type type;
	char *name;
};

extern struct instruction_data instruction_data[];

void print_code(FILE *f, struct instruction *code, size_t num_instr);

#endif
