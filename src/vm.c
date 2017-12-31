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
push_frame(struct vm *vm)
{
	vm->frame = oak_realloc(vm->frame, (vm->fp + 1) * sizeof *vm->frame);
	vm->frame[vm->fp] = oak_malloc(256 * sizeof *vm->frame[vm->fp]);
	vm++;
}

#define REG(X) vm->frame[vm->fp][X]

void
execute_instr(struct vm *vm, struct instruction c)
{
	if (vm->debug) {
		fprintf(vm->f, "> %3zu: ", vm->ip);
		print_instruction(vm->f, c);
		fputc('\n', vm->f);
	}

	switch (c.type) {
	case INSTR_MOVC:
		/* Look at this c.d.bc.c bullshit. Ridiculous. */
		REG(c.d.bc.b) = vm->ct->val[c.d.bc.c];
		break;

	case INSTR_PRINT:
		print_value(vm->f, vm->gc, vm->frame[vm->fp][c.d.a]);
		if (vm->debug) fputs("\n", vm->f);
		break;

	case INSTR_LINE:
		if (!vm->debug) fputc('\n', vm->f);
		break;

	case INSTR_ADD:
		REG(c.d.efg.e) = add_values(vm->gc, REG(c.d.efg.f), REG(c.d.efg.g));
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
	vm->code = m->code;
	vm->debug = debug;
	vm->ct = m->ct;
	vm->gc = m->gc;
	vm->f = stderr;
	push_frame(vm);

	while (vm->code[vm->ip].type != INSTR_END) {
		execute_instr(vm, c[vm->ip]);
		vm->ip++;
	}

	for (size_t i = 0; i < vm->fp; i++) {
		free(vm->frame[i]);
	}

	free(vm->frame);
}

void
vm_panic(struct vm *vm)
{
	/* TODO: clean up memory stuff */
	error_write(vm->r, stderr);
	exit(EXIT_FAILURE);
}
