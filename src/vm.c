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
	vm->fp++;
	if (vm->fp <= vm->maxfp) return;
	vm->frame = oak_realloc(vm->frame, (vm->fp + 1) * sizeof *vm->frame);
	vm->frame[vm->fp] = oak_malloc(256 * sizeof *vm->frame[vm->fp]);
	vm->maxfp = vm->fp > vm->maxfp ? vm->fp : vm->maxfp;
}

static void
pop_frame(struct vm *vm)
{
	vm->fp--;
}

static void
push(struct vm *vm, struct value v)
{
	vm->sp++;

	if (vm->sp <= vm->maxsp) {
		vm->stack[vm->sp] = v;
		return;
	}

	vm->stack = oak_realloc(vm->stack, (vm->sp + 1) * sizeof *vm->stack);
	vm->stack[vm->sp] = v;
	vm->maxsp = vm->sp > vm->maxsp ? vm->sp : vm->maxsp;
}

static struct value
pop(struct vm *vm)
{
	return vm->stack[vm->sp--];
}

static void
call(struct vm *vm, struct value v)
{
	/* TODO: Optimize everything. Lol. */
	assert(v.type == VAL_FN);
	push_frame(vm);
	vm->callstack = oak_realloc(vm->callstack, (vm->csp + 2) * sizeof *vm->callstack);
	vm->csp++;
	vm->callstack[vm->csp] = vm->ip;
	vm->ip = v.integer - 1;
}

static void
ret(struct vm *vm)
{
	assert(vm->csp != 0);
	pop_frame(vm);
	vm->ip = vm->callstack[vm->csp--];
}

#define REG(X) vm->frame[vm->fp][X]
#define GLOBAL(X) vm->frame[1][X]
#define CONST(X) vm->ct->val[X]
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
	case INSTR_MOVC: REG(c.d.bc.b) = CONST(c.d.bc.c);       break;
	case INSTR_LINE: if (!vm->debug) fputc('\n', vm->f);    break;
	case INSTR_MOV:  REG(c.d.bc.b) = REG(c.d.bc.c);         break;
	case INSTR_JMP:  vm->ip = c.d.d - 1;                    break;
	case INSTR_PUSH: push(vm, REG(c.d.a));                  break;
	case INSTR_POP:  REG(c.d.a) = pop(vm);                  break;
	case INSTR_CALL: call(vm, REG(c.d.a));                  break;
	case INSTR_RET:  ret(vm);                               break;
	case INSTR_ADD:  BIN(add);                              break;
	case INSTR_SUB:  BIN(sub);                              break;
	case INSTR_MUL:  BIN(mul);                              break;
	case INSTR_DIV:  BIN(div);                              break;
	case INSTR_MOD:  BIN(mod);                              break;
	case INSTR_OR:   BIN(or);                               break;
	case INSTR_GMOV: GLOBAL(c.d.bc.b) = REG(c.d.bc.c);      break;
	case INSTR_MOVG: REG(c.d.bc.b) = GLOBAL(c.d.bc.c);      break;
	case INSTR_INC:  REG(c.d.a) = inc_value(vm->gc, REG(c.d.a)); break;
	case INSTR_GINC: GLOBAL(c.d.a) = inc_value(vm->gc, GLOBAL(c.d.a)); break;
	case INSTR_SUBSCR:
		if (REG(c.d.efg.g).integer >= vm->gc->arrlen[REG(c.d.efg.f).idx]
		    || REG(c.d.efg.g).integer < 0) {
			struct value v;
			v.type = VAL_NIL;
			REG(c.d.efg.e) = v;
			break;
		}

		REG(c.d.efg.e) = copy_value(vm->gc, vm->gc->array[REG(c.d.efg.f).idx]
		                            [REG(c.d.efg.g).integer]);
		break;

	case INSTR_NEG:   REG(c.d.bc.b) = neg_value(vm->gc, REG(c.d.bc.c)); break;
	case INSTR_COPY:  REG(c.d.bc.b) = copy_value(vm->gc, REG(c.d.bc.c)); break;
	case INSTR_COPYC: REG(c.d.bc.b) = copy_value(vm->gc, CONST(c.d.bc.c)); break;
	case INSTR_COPYG: REG(c.d.bc.b) = copy_value(vm->gc, GLOBAL(c.d.bc.c)); break;

	case INSTR_PUSHBACK:
		REG(c.d.bc.b) = pushback(vm->gc,
		                         REG(c.d.bc.b),
		                         REG(c.d.bc.c));
		break;

	case INSTR_ASET:
		/*
		 * TODO: Somewhere we should make sure that the index
		 * is actually an integer.
		 */

		assert(REG(c.d.efg.e).type == VAL_ARRAY);
		assert(REG(c.d.efg.f).type == VAL_INT);
		grow_array(vm->gc, REG(c.d.efg.e), REG(c.d.efg.f).integer + 1);

		if (vm->gc->arrlen[REG(c.d.efg.e).idx] <= REG(c.d.efg.f).integer) {
			vm->gc->arrlen[REG(c.d.efg.e).idx] = REG(c.d.efg.f).integer + 1;
		}

		vm->gc->array[REG(c.d.efg.e).idx][REG(c.d.efg.f).integer]
			= REG(c.d.efg.g);
		break;

	case INSTR_GASET:
		assert(GLOBAL(c.d.efg.e).type == VAL_ARRAY);
		assert(REG(c.d.efg.f).type == VAL_INT);
		grow_array(vm->gc, GLOBAL(c.d.efg.e), REG(c.d.efg.f).integer + 1);

		if (vm->gc->arrlen[GLOBAL(c.d.efg.e).idx] <= REG(c.d.efg.f).integer) {
			vm->gc->arrlen[GLOBAL(c.d.efg.e).idx] =
				REG(c.d.efg.f).integer + 1;
		}

		vm->gc->array[GLOBAL(c.d.efg.e).idx][REG(c.d.efg.f).integer]
			= REG(c.d.efg.g);
		break;

	case INSTR_DEREF:
		grow_array(vm->gc, REG(c.d.efg.f), REG(c.d.efg.g).integer + 1);

		if (vm->gc->array[REG(c.d.efg.f).idx][REG(c.d.efg.g).integer].type == VAL_ARRAY) {
			REG(c.d.efg.e) = vm->gc->array[REG(c.d.efg.f).idx]
				                      [REG(c.d.efg.g).integer];
		} else {
			struct value v;
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->arrlen[v.idx] = 0;
			vm->gc->array[v.idx] = NULL;
			vm->gc->array[REG(c.d.efg.f).idx][REG(c.d.efg.g).integer] = v;
			REG(c.d.efg.e) = v;
		}
		break;

	case INSTR_GDEREF:
		grow_array(vm->gc, GLOBAL(c.d.efg.f), REG(c.d.efg.g).integer + 1);

		if (vm->gc->array[REG(c.d.efg.f).idx][REG(c.d.efg.g).integer].type == VAL_ARRAY) {
			REG(c.d.efg.e) = vm->gc->array[GLOBAL(c.d.efg.f).idx]
				                      [REG(c.d.efg.g).integer];
		} else {
			struct value v;
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->arrlen[v.idx] = 0;
			vm->gc->array[v.idx] = NULL;
			vm->gc->array[GLOBAL(c.d.efg.f).idx][REG(c.d.efg.g).integer] = v;
			REG(c.d.efg.e) = v;
		}
		break;

	case INSTR_PRINT:
		print_value(vm->f, vm->gc, vm->frame[vm->fp][c.d.a]);
		if (vm->debug) fputc('\n', vm->f);
		break;

	case INSTR_COND:
		if (is_truthy(vm->gc, REG(c.d.a)))
			vm->ip++;
		break;

	case INSTR_NCOND:
		if (!is_truthy(vm->gc, REG(c.d.a)))
			vm->ip++;
		break;

	case INSTR_CMP:
		REG(c.d.efg.e) = cmp_values(vm->gc, REG(c.d.efg.f), REG(c.d.efg.g));
		break;

	case INSTR_LESS:
		REG(c.d.efg.e) = value_less(vm->gc, REG(c.d.efg.f), REG(c.d.efg.g));
		break;

	case INSTR_TYPE: {
		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = oak_malloc(strlen(value_data[REG(c.d.bc.c).type].body) + 1);
		strcpy(vm->gc->str[v.idx], value_data[REG(c.d.bc.c).type].body);
		REG(c.d.bc.b) = v;
	} break;

	case INSTR_LEN:
		REG(c.d.bc.b) = value_len(vm->gc, REG(c.d.bc.c));
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
