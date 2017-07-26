#ifndef VM_H
#define VM_H

#include "machine.h"
#include "constant.h"
#include "module.h"
#include "compile.h"
#include "code.h"
#include "value.h"
#include "error.h"

void execute(struct module *m);
void vm_panic(struct vm *vm);

#endif
