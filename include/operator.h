#ifndef OPERATOR_H
#define OPERATOR_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct Operator {
	char	*body;
	char	*body2;
	size_t	 prec;

	enum BinaryAssociation {
		ASS_LEFT,
		ASS_RIGHT
	} ass;

	enum OpType {
		/* prefix operator types */
		OP_PREFIX,
		OP_GROUP, /* only used for grouping with () */

		/* infix operator types */
		OP_POSTFIX,
		OP_BINARY,
		OP_MEMBER,
		OP_TERNARY,
		OP_INVALID
	} type;
};

extern struct Operator ops[];
size_t num_ops();

#endif
