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

static void
alloc_frame(struct vm *vm)
{
	vm->frame = oak_realloc(vm->frame, vm->fp * sizeof *vm->frame);

	if (!vm->frame[vm->fp]) {
		vm->frame[vm->fp] = oak_malloc(256 * sizeof *vm->frame[vm->fp]);
	}
}

void
execute_instr(struct vm *vm, struct instruction c)
{
	if (vm->debug)
		DOUT("%3zu: %s (%d)\n", vm->ip, instruction_data[c.type].name, c.d.a);

	switch (c.type) {
	case INSTR_MOVC:
		/*
		 * All we need to do is make sure the registers are
		 * allocated.
		 */
		alloc_frame(vm);

		/* Look at this c.d.bc.c bullshit. Ridiculous. */
		vm->frame[vm->fp][c.d.bc.b] = vm->ct->val[c.d.bc.c];
		break;

	case INSTR_PRINT:
		print_value(stdout, vm->gc, vm->frame[vm->fp][c.d.a]);
		break;

	case INSTR_LINE:
		puts("");
		break;

	default:
		DOUT("unimplemented instruction %d (%s)", c.type,
		     instruction_data[c.type].name);
		assert(false);
	}
}

void
execute(struct module *m, bool debug)
{
	struct vm *vm = new_vm();
	struct instruction *c = m->code;
	vm->ct = m->ct;
	vm->code = m->code;
	vm->gc = m->gc;
	vm->debug = debug;
	vm->fp = 1;
	alloc_frame(vm);

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
