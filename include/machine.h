#ifndef MACHINE_H
#define MACHINE_H

#include <stdlib.h>
#include "constant.h"

struct frame {
	struct value *vars;
	size_t num;
};

struct vm {
	struct instruction *code;
	size_t ip;

	struct value *stack;
	size_t sp;

	struct frame *frames;
	size_t fp;

	struct constant_table constant_table;

	struct reporter *r;
};

#endif
