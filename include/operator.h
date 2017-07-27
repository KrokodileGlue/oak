#ifndef OPERATOR_H
#define OPERATOR_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct operator {
	char   *body;
	char   *body2;
	size_t  prec;

	enum {
		ASS_LEFT,
		ASS_RIGHT
	} ass;

	enum operator_type {
		/* prefix operator types */
		OPTYPE_PREFIX,

		/* infix operator types */
		OPTYPE_FN_CALL,
		OPTYPE_POSTFIX,
		OPTYPE_BINARY,
		OPTYPE_TERNARY,
		OPTYPE_SUBCRIPT,
		OPTYPE_INVALID
	} type;

	enum {
		OP_LBRACE, OP_PERIOD, OP_LBRACK, OP_ADDADD, OP_SUBSUB,
		OP_EXCLAMATION, OP_ADD, OP_SUB, OP_TYPE, OP_LENGTH,
		OP_MUL, OP_DIV, OP_MOD, OP_ARROW, OP_LESS, OP_MORE,
		OP_LEQ, OP_GEQ, OP_EQEQ, OP_AND, OP_OR, OP_QUESTION,
		OP_EQ, OP_ADDEQ, OP_SUBEQ, OP_COMMA, OP_NOTEQ, OP_SAY
	} name;
};

extern struct operator ops[];

size_t
num_ops();

#endif
