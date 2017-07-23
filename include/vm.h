#ifndef VM_H
#define VM_H

#include "constant.h"
#include "module.h"
#include "compile.h"
#include "code.h"
#include "value.h"

struct vm {
	size_t ip;
	struct value *stack;
	size_t sp;

	struct constant_table constant_table;
};

void execute(struct module *m);

#endif
