#include <inttypes.h>
#include <assert.h>

#include "util.h"
#include "vm.h"

static struct vm *
new_vm()
{
	struct vm *v = oak_malloc(sizeof *v);

	memset(v, 0, sizeof *v);

	return v;
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

static void
print(struct value v)
{
	switch (v.type) {
	case VAL_INT:
		fprintf(stdout, "%"PRId64, v.integer);
		break;
	case VAL_STR:
		fprintf(stdout, "%s", v.str.text);
		break;
	case VAL_FLOAT:
		fprintf(stdout, "%f", v.real);
		break;
	default:
		DOUT("unimplemented printer for value of type %d (%s)", v.type, value_data[v.type].body);
		assert(false);
	}
}

void
execute_instr(struct vm *vm, struct instruction c)
{
//	fprintf(stderr, "%s\n", instruction_data[c.type].body);

	switch (c.type) {
	case INSTR_PUSH_CONST: {
		push(vm, vm->constant_table.val[c.a]);
	} break;

	case INSTR_PRINT: {
		struct value v = pop(vm);
		print(v);
	} break;

	case INSTR_ADD: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = add_values(l, r);
		push(vm, ans);
	} break;

	case INSTR_SUB: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = sub_values(l, r);
		push(vm, ans);
	} break;

	case INSTR_MUL: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = mul_values(l, r);
		push(vm, ans);
	} break;

	case INSTR_DIV: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = div_values(l, r);
		push(vm, ans);
	} break;

	case INSTR_PUSH_LOCAL: {
		push(vm, vm->frames[vm->fp].vars[c.a]);
	} break;

	case INSTR_POP_LOCAL: {
		// HACK: very hacky.
		if (vm->frames[vm->fp].num < c.a) {
			vm->frames[vm->fp].num  = c.a;
		}

		vm->frames[vm->fp].vars = realloc(vm->frames[vm->fp].vars, sizeof vm->frames->vars[0] * (vm->frames[vm->fp].num + 1));
		vm->frames[vm->fp].vars[c.a] = pop(vm);
	} break;

	case INSTR_LINE: putchar('\n'); break;

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
