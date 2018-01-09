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
	if (!vm) return;

	for (size_t i = 1; i < vm->maxfp + 1; i++)
		free(vm->frame[i]);

	error_clear(vm->r);

	if (vm->frame) free(vm->frame);
	if (vm->stack) free(vm->stack);
	if (vm->callstack) free(vm->callstack);
	if (vm->imp) free(vm->imp);
	if (vm->subject) free(vm->subject);

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
	if (v.type != VAL_FN) {
		error_push(vm->r, *vm->code[vm->ip].loc, ERR_FATAL,
		           "attempt to call a non-callable object as function");
		return;
	}

	if (vm->csp >= MAX_CALL_DEPTH - 1) {
		error_push(vm->r, *vm->code[vm->ip].loc, ERR_FATAL,
		           "program exceeded the maximum call depth of %d",
		           MAX_CALL_DEPTH);
		return;
	}

	if (vm->debug)
		fprintf(stderr, "<function call : `%s' : %p : %zu argument%s>\n",
		        v.name ? v.name : "nameless function", (void *)&vm->code[vm->ip], vm->sp,
		        vm->sp == 1 ? "" : "s");

	push_frame(vm);
	vm->csp++;

	vm->callstack = oak_realloc(vm->callstack, (vm->csp + 1) * sizeof *vm->callstack);
	vm->callstack[vm->csp] = vm->ip;

	vm->args[vm->csp] = vm->sp;
	vm->calls[vm->csp] = v;

	vm->ip = v.integer - 1;
}

static void
ret(struct vm *vm)
{
	assert(vm->csp != 0);
	pop_frame(vm);
	vm->ip = vm->callstack[vm->csp--];
}

static void
stacktrace(struct vm *vm)
{
	fprintf(stderr, "Stack trace:\n");
	size_t depth = 0;
	if (vm->csp > 10) depth = vm->csp - 10;

	for (size_t i = vm->csp; i > depth; i--) {
		struct instruction c = vm->code[vm->callstack[i]];
		fprintf(stderr, "\t%2zu: <`%10s' : %p : %d argument%s>",
		        i, vm->calls[i].name, (void *)&vm->code[vm->callstack[i]],
		        vm->args[i], vm->args[i] == 1 ? "" : "s");
		fprintf(stderr, "@%"PRIu64" ", vm->calls[i].integer);
		fprintf(stderr, "%s:%zu:%zu\n",
		        c.loc->file, line_number(*c.loc), column_number(*c.loc));
	}

	if (depth != 0)
		fprintf(stderr, "\t--- Truncated ---\n");
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
	case INSTR_POW:  BIN(pow);                              break;
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

	case INSTR_FLIP:  REG(c.d.bc.b) = flip_value(vm->gc, REG(c.d.bc.c)); break;
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

		if (REG(c.d.efg.e).type != VAL_ARRAY) {
			REG(c.d.efg.e).type = VAL_ARRAY;
			REG(c.d.efg.e).idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->arrlen[REG(c.d.efg.e).idx] = 0;
			vm->gc->array[REG(c.d.efg.e).idx] = NULL;
		}

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

	case INSTR_MORE:
		REG(c.d.efg.e) = value_more(vm->gc, REG(c.d.efg.f), REG(c.d.efg.g));
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

	case INSTR_KILL:
		assert(REG(c.d.a).type == VAL_STR);
		error_push(vm->r, *c.loc, ERR_KILLED, vm->gc->str[REG(c.d.a).idx]);
		break;

	case INSTR_PUSHIMP:
		vm->imp = oak_realloc(vm->imp, (vm->impp + 1) * sizeof *vm->imp);
		vm->imp[vm->impp++] = REG(c.d.a);
		break;

	case INSTR_POPIMP:
		vm->impp--;
		break;

	case INSTR_GETIMP:
		REG(c.d.a) = vm->imp[vm->impp - 1];
		break;

	case INSTR_MATCH: {
		if (REG(c.d.efg.g).type != VAL_REGEX) {
			error_push(vm->r, *c.loc, ERR_FATAL, "attempt to apply match to non regular expression value");
			return;
		}

		if (REG(c.d.efg.f).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL, "attempt to apply regular expression to non-string value");
			return;
		}

		if (vm->subject) free(vm->subject);

		int **vec = NULL;
		struct ktre *re = vm->gc->regex[REG(c.d.efg.g).idx];
		char *subject = vm->gc->str[REG(c.d.efg.f).idx];
		bool ret = ktre_exec(re, subject, &vec);

		vm->subject = oak_malloc(strlen(subject) + 1);
		strcpy(vm->subject, subject);
		vm->re = re;

		REG(c.d.efg.e).type = VAL_ARRAY;
		REG(c.d.efg.e).idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[REG(c.d.efg.e).idx] = NULL;
		vm->gc->arrlen[REG(c.d.efg.e).idx] = 0;

		if (ret) {
			for (int i = 0; i < re->num_matches; i++) {
				struct value v;
				v.type = VAL_STR;
				v.idx = gc_alloc(vm->gc, VAL_STR);
				vm->gc->str[v.idx] = oak_malloc(vec[i][1] + 1);
				strncpy(vm->gc->str[v.idx], subject + vec[i][0], vec[i][1]);
				vm->gc->str[v.idx][vec[i][1]] = 0;
				pushback(vm->gc, REG(c.d.efg.e), v);

				for (int j = 1; j < re->num_groups; j++) {
					/* TODO: $1, $2 n stuff */
					/* DOUT("\ngroup %d: `%.*s`", j, vec[i][j * 2 + 1], subject + vec[i][j * 2]); */
				}
			}
		} else if (re->err) {
			/* TODO: Make the runtime fail n stuff. */
			fprintf(stderr, "\nfailed at runtime with error code %d: %s\n",
			        re->err, re->err_str ? re->err_str : "no error message");
			fprintf(stderr, "\t%s\n\t", re->pat);
			for (int i = 0; i < re->loc; i++) fprintf(stderr, " ");
			fprintf(stderr, "^");
		}

		/* for (int i = 0; i < re->num_matches; i++) */
		/* 	free(vec[i]); */

		/* free(vec); */
	} break;

	case INSTR_GROUP: {
		if (REG(c.d.bc.c).type != VAL_INT)
			assert(false);

		REG(c.d.bc.b).type = VAL_STR;
		int i = vm->re->num_matches - 1;
		int **vec = ktre_getvec(vm->re);

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);

		vm->gc->str[v.idx] = oak_malloc(vec[i][REG(c.d.bc.c).integer * 2 + 1] + 1);
		strncpy(vm->gc->str[v.idx], vm->subject + vec[i][REG(c.d.bc.c).integer * 2], vec[i][REG(c.d.bc.c).integer * 2 + 1]);
		vm->gc->str[v.idx][vec[i][REG(c.d.bc.c).integer * 2 + 1]] = 0;

		for (int i = 0; i < vm->re->num_matches; i++)
			free(vec[i]);

		free(vec);
		REG(c.d.bc.b) = v;
	} break;

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
		if (vm->r->pending) break;
		vm->ip++;
	}

	if (vm->r->pending) {
		error_write(vm->r, stderr);
		if (vm->csp) stacktrace(vm);
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
