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

static void
push_frame(struct vm *vm, size_t num_variables)
{
	/* HACK: more unnecessary reallocing. */
	vm->frames = realloc(vm->frames, sizeof vm->frames[0] * (vm->fp + 1));
	vm->frames[vm->fp++] = new_frame(num_variables);
}

static void
pop_frame(struct vm *vm)
{
	vm->fp--;
}

static void
vm_list(struct vm *vm)
{
	struct value val;
	val.type = VAL_LIST;
	val.list = new_list();

	/* TODO: verify that we actually have an integer here. */
	struct value len = pop(vm);
	for (int64_t i = 0; i < len.integer; i++) {
		struct value key = pop(vm);
		struct value value = pop(vm);

		list_insert(val.list, &key.integer, sizeof key.integer, value);
	}

	push(vm, val);
}

void
execute_instr(struct vm *vm, struct instruction c)
{
//	fprintf(stderr, "%3zu: %s (%zu)\n", vm->ip, instruction_data[c.type].body, c.arg);

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

	case INSTR_ASSIGN: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		assert(l.type == VAL_INT);
		struct value subscript = pop(vm);
		struct value *lvalue = &vm->frames[vm->fp - 1]->vars[l.integer];

		struct value *val = lvalue;
		for (int64_t i = 0; i < subscript.integer; i++) {
			struct value *old_val = val;
			struct value key = pop(vm);
			if (val->type != VAL_LIST) {
				val->type = VAL_LIST;
				val->list = new_list();
			}
			val = list_lookup(val->list, &key.integer, sizeof key.integer);

			if (!val) {
				struct value list;
				list.type = VAL_LIST;
				list.list = new_list();
				list_insert(old_val->list, &key.integer, sizeof key.integer, list);

				val = list_lookup(old_val->list, &key.integer, sizeof key.integer);
			}
		}

		if (subscript.integer == 0) {
			vm->frames[vm->fp - 1]->vars[l.integer] = r;
			push(vm, r);
		} else *val = r;

		push(vm, r);
	} break;
	case INSTR_RANGE: {
		struct value step = pop(vm);
		struct value r = pop(vm);
		struct value l = pop(vm);

		assert(step.type == VAL_INT);
		assert(l.type == VAL_INT);
		assert(r.type == VAL_INT);

		int64_t n = 0;
		int64_t start = r.integer;
		int64_t stop = l.integer;
		int64_t m = step.integer;
		int64_t total = 0;

		if (start >= stop) {
			total = (start - stop) / m;
			for (int64_t i = start; i >= stop; i -= m) {
				struct value val;
				val.type = VAL_INT;

				/* push the value */
				val.integer = i;
				push(vm, val);

				/* push the key */
				val.integer = total - n;
				push(vm, val);
				n++;
			}
		} else {
			total = (stop - start) / m;
			for (int64_t i = start; i <= stop; i += m) {
				struct value val;
				val.type = VAL_INT;

				/* push the value */
				val.integer = i;
				push(vm, val);

				/* push the key */
				val.integer = total - n;
				push(vm, val);
				n++;
			}
		}

		struct value num = (struct value){VAL_INT, {n}};
		push(vm, num);

		vm_list(vm);
	} break;

	case INSTR_INC: {
		struct value l = pop(vm);
		l = inc_value(vm, l);
		push(vm, l);
	} break;

	case INSTR_NEG: {
		struct value l = pop(vm);
		l = neg_value(vm, l);
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

	case INSTR_LEQ: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		bool is = is_less_than_value(vm, l, r).boolean || cmp_values(vm, l, r).boolean;
		struct value ans = (struct value){VAL_BOOL, {is}};
		push(vm, ans);
	} break;

	case INSTR_MORE: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = is_more_than_value(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_LEN: {
		struct value l = pop(vm);
		push(vm, len_value(vm, l));
	} break;

	case INSTR_SAY: {
		struct value l = pop(vm);
		print_value(stdout, l);
		push(vm, l);
	} break;

	case INSTR_TYPE: {
		struct value l = pop(vm);
		struct value ret;
		ret.type = VAL_STR;
		ret.str.text = strclone(value_data[l.type].body);
		ret.str.len = strlen(ret.str.text);
		push(vm, ret);
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

	case INSTR_FLIP: {
		struct value l = pop(vm);
		push(vm, flip_value(vm, l));
	} break;

	case INSTR_AND: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = and_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_OR: {
		struct value r = pop(vm);
		struct value l = pop(vm);
		struct value ans = or_values(vm, l, r);
		push(vm, ans);
	} break;

	case INSTR_PUSH_LOCAL: {
		push(vm, vm->frames[vm->fp - 1]->vars[c.arg]);
	} break;

	case INSTR_POP_LOCAL: {
		vm->frames[vm->fp - 1]->vars[c.arg] = pop(vm);
	} break;

	case INSTR_POP: {
		pop(vm);
	} break;

	case INSTR_LINE: putchar('\n'); break;

	case INSTR_COND_JUMP: {
		struct value l = pop(vm);
		if (is_value_true(vm, l))
			vm->ip = c.arg - 1;
	} break;

	case INSTR_FALSE_JUMP: {
		struct value l = pop(vm);
		if (!is_value_true(vm, l))
			vm->ip = c.arg;
	} break;

	case INSTR_TRUE_JUMP: {
		struct value l = pop(vm);
		if (is_value_true(vm, l))
			vm->ip = c.arg;
	} break;

	case INSTR_JUMP: {
		vm->ip = c.arg - 1;
	} break;

	case INSTR_CALL: {
		struct value addr = pop(vm);
		assert(addr.type == VAL_INT);

		vm->frames[vm->fp - 1]->address = vm->ip;
		vm->ip = addr.integer - 1;
	} break;

	case INSTR_RET: {
		pop_frame(vm);
		struct value ret = pop(vm);

		vm->ip = vm->frames[vm->fp - 1]->address;
		push(vm, ret);
	} break;

	case INSTR_FRAME: {
		struct value val = pop(vm);
		push_frame(vm, val.integer);
	} break;

	case INSTR_POP_FRAME: {
		pop_frame(vm);
	} break;

	case INSTR_LIST: {
		vm_list(vm);
	} break;

	case INSTR_SUBSCRIPT: {
		struct value key = pop(vm);
		struct value list = pop(vm);

		/* TODO: more types of keys */
		uint64_t i = list_index(list.list, &key.integer, sizeof key.integer);

		if (i == (uint64_t)-1) {
			struct value val = (struct value){VAL_NIL, { 0 }};
			push(vm, val);
			break;
		}

		push(vm, list.list->val[i]);
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
