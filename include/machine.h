#ifndef MACHINE_H
#define MACHINE_H

#include <stdlib.h>
#include "constant.h"
#include "gc.h"

struct vm {
	struct instruction *code;
	size_t ip;

	struct value *stack;
	size_t sp;

	struct value **frame;
	size_t fp;

	struct constant_table *ct;
	struct reporter *r;
	struct gc *gc;

	bool debug;
	FILE *f;
};

#endif
