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
free_vm(struct vm *vm)
{
	for (size_t i = 1; i < vm->maxfp + 1; i++) {
		free(vm->frame[i]);
	}

	error_clear(vm->r);
	free(vm->frame);
	free(vm->stack);
	free(vm->callstack);
	free(vm);
}

static void
push_frame(struct vm *vm)
{
	vm->frame = oak_realloc(vm->frame, (vm->fp + 2) * sizeof *vm->frame);
	vm->fp++;
	vm->frame[vm->fp] = oak_malloc(256 * sizeof *vm->frame[vm->fp]);
	vm->maxfp = vm->fp > vm->maxfp ? vm->fp : vm->maxfp;
}

static void
push(struct vm *vm, struct value v)
{
	vm->stack = oak_realloc(vm->stack, (vm->sp + 2) * sizeof *vm->stack);
	vm->sp++;
	vm->stack[vm->sp] = v;
}

static struct value
pop(struct vm *vm)
{
	return vm->stack[vm->sp--];
}

static void
call(struct vm *vm, int ip)
{
	vm->callstack = oak_realloc(vm->callstack, (vm->csp + 2) * sizeof *vm->callstack);
	vm->csp++;
	vm->callstack[vm->csp] = vm->ip;
	vm->ip = ip;
}

static void
ret(struct vm *vm)
{
	vm->ip = vm->callstack[vm->csp--];
}

#define REG(X) vm->frame[vm->fp][X]
#define BIN(X) REG(c.d.efg.e) = X##_values(vm->gc, REG(c.d.efg.f), REG(c.d.efg.g))

void
execute_instr(struct vm *vm, struct instruction c)
{
	if (vm->debug) {
		fprintf(vm->f, "> %3zu: ", vm->ip);
		print_instruction(vm->f, c);
		fputc('\n', vm->f);
	}

	/* Look at all this c.d.bc.c bullshit. Ridiculous. */

	switch (c.type) {
	case INSTR_MOVC: REG(c.d.bc.b) = vm->ct->val[c.d.bc.c]; break;
	case INSTR_LINE: if (!vm->debug) fputc('\n', vm->f);    break;
	case INSTR_MOV:  REG(c.d.bc.b) = REG(c.d.bc.c);         break;
	case INSTR_JMP:  vm->ip = c.d.a - 1;                    break;
	case INSTR_PUSH: push(vm, REG(c.d.a));                  break;
	case INSTR_POP:  REG(c.d.a) = pop(vm);                  break;
	case INSTR_CALL: call(vm, c.d.a - 1);                   break;
	case INSTR_RET:  ret(vm);                               break;
	case INSTR_ADD:  BIN(add);                              break;
	case INSTR_SUB:  BIN(sub);                              break;
	case INSTR_MUL:  BIN(mul);                              break;
	case INSTR_DIV:  BIN(div);                              break;

	case INSTR_PUSHBACK:
		break;

	case INSTR_PRINT:
		print_value(vm->f, vm->gc, vm->frame[vm->fp][c.d.a]);
		if (vm->debug) fputc('\n', vm->f);
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

	free_vm(vm);
}

void
vm_panic(struct vm *vm)
{
	/* TODO: clean up memory stuff */
	error_write(vm->r, stderr);
	exit(EXIT_FAILURE);
}
