#ifndef MACHINE_H
#define MACHINE_H

#include <stdlib.h>
#include "constant.h"
#include "gc.h"

struct vm {
	struct instruction *code;
	size_t ip;

	/* Return locations. */
	int *callstack;
	/* Detailed information about each call. */
	struct value *calls;
	int *args;
	size_t csp;

	struct value *stack;
	size_t sp;
	size_t maxsp;

	struct value **frame;
	size_t fp;
	size_t maxfp;

	struct constant_table *ct;
	struct reporter *r;
	struct gc *gc;

	bool debug;
	FILE *f;
};

#endif
