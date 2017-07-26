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

static struct value
pop(struct vm *vm)
{
	struct value v = vm->stack[--(vm->sp)];
	return v;
}

static void
push(struct vm *vm, struct value val)
{
	/* this is all temporary. */
	vm->stack = realloc(vm->stack, sizeof vm->stack[0] * (vm->sp + 2));
	vm->stack[vm->sp++] = val;
}

void
execute_instr(struct vm *vm, struct instruction c)
{
//	fprintf(stderr, "%zu: %s\n", vm->ip, instruction_data[c.type].body);

	switch (c.type) {
	case INSTR_PUSH_CONST: {
		push(vm, vm->constant_table.val[c.arg]);
	} break;

	case INSTR_PRINT: {
		struct value v = pop(vm);
		print_value(stdout, v);
	} break;

	case INSTR_ADD: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = add_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_SUB: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = sub_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_MUL: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = mul_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_DIV: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = div_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_INC: {
		struct value l = pop(vm);
		l = inc_value(vm, l);
		push(vm, l);
	} break;

	case INSTR_DEC: {
		struct value l = pop(vm);
		l = dec_value(vm, l);
		push(vm, l);
	} break;

	case INSTR_LESS: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = is_less_than_value(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_MOD: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = mod_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_CMP: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = cmp_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_AND: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = and_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_PUSH_LOCAL: {
		push(vm, vm->frames[vm->fp].vars[c.arg]);
	} break;

	case INSTR_POP_LOCAL: {
		// HACK: very hacky.
		if (vm->frames[vm->fp].num < c.arg) {
			vm->frames[vm->fp].num  = c.arg;
		}

		vm->frames[vm->fp].vars = realloc(vm->frames[vm->fp].vars, sizeof vm->frames->vars[0] * (vm->frames[vm->fp].num + 1));
		vm->frames[vm->fp].vars[c.arg] = pop(vm);
	} break;

	case INSTR_POP: {
		pop(vm);
	} break;

	case INSTR_LINE: putchar('\n'); break;

	case INSTR_COND_JUMP: {
		struct value l = pop(vm);
		if (is_value_true(vm, l))
			vm->ip = c.arg;
	} break;

	case INSTR_FALSE_JUMP: {
		struct value l = pop(vm);
		if (!is_value_true(vm, l))
			vm->ip = c.arg;
	} break;

	case INSTR_JUMP: {
		vm->ip = c.arg;
	} break;

	default:
		DOUT("unimplemented instruction %d (%s)", c.type, instruction_data[c.type].body);
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

	vm->frames = oak_malloc(sizeof vm->frames[0]);
	memset(vm->frames, 0, sizeof vm->frames[0]);

	while (vm->code[vm->ip].type != INSTR_END) {
		execute_instr(vm, c[vm->ip]);
		vm->ip++;
	}
}

void
vm_panic(struct vm *vm)
{
	/* TODO: clean up the vm's memory */
	error_write(vm->r, stderr);
	exit(EXIT_FAILURE);
}
