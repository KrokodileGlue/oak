#ifndef VM_H
#define VM_H

#include "machine.h"
#include "constant.h"
#include "module.h"
#include "compile.h"
#include "code.h"
#include "value.h"
#include "error.h"

struct vm *new_vm(struct module *m, struct oak *k, bool debug);
void execute(struct vm *vm, int64_t ip);
void vm_panic(struct vm *vm);

#endif
