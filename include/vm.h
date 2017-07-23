#ifndef VM_H
#define VM_H

#include "constant.h"
#include "module.h"
#include "compile.h"
#include "code.h"
#include "value.h"

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
};

void execute(struct module *m);

#endif
