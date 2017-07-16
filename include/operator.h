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
		OP_PREFIX,

		/* infix operator types */
		OP_FN_CALL,
		OP_POSTFIX,
		OP_BINARY,
		OP_TERNARY,
		OP_SUBCRIPT,
		OP_INVALID
	} type;
};

extern struct operator ops[];

size_t
num_ops();

#endif
