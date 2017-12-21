#include <inttypes.h>
#include <assert.h>

#include "util.h"
#include "vm.h"

static struct vm *
new_vm()
{
	struct vm *vm = oak_malloc(sizeof *vm);

	memset(vm, 0, sizeof *vm);
	vm->r = new_reporter();

	return vm;
}

void
execute_instr(struct vm *vm, struct instruction c)
{
	fprintf(stderr, "%3zu: %s (%d)\n", vm->ip, instruction_data[c.type].name, c.d.a);

	switch (c.type) {
	default:
		DOUT("unimplemented instruction %d (%s)", c.type, instruction_data[c.type].name);
		assert(false);
	}
}

void
execute(struct module *m)
{
	struct vm *vm = new_vm();
	struct instruction *c = m->code;
	vm->constant_table = m->constant_table;
	vm->code = m->code;

	while (vm->code[vm->ip].type != INSTR_END) {
		execute_instr(vm, c[vm->ip]);
		vm->ip++;
	}
}

void
vm_panic(struct vm *vm)
{
	/* TODO: clean up memory stuff */
	error_write(vm->r, stderr);
	exit(EXIT_FAILURE);
}
