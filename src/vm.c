#include <inttypes.h>
#include <assert.h>

#include "util.h"
#include "vm.h"

void
push_frame(struct vm *vm)
{
	vm->fp++;

	if (vm->fp <= vm->maxfp) {
		vm->module[vm->fp] = vm->m->id;

		for (int i = 0; i < NUM_REG; i++)
			vm->frame[vm->fp][i].type = VAL_UNDEF;
		return;
	}

	vm->frame = oak_realloc(vm->frame, (vm->fp + 1) * sizeof *vm->frame);
	vm->frame[vm->fp] = oak_malloc(NUM_REG * sizeof *vm->frame[vm->fp]);

	for (int i = 0; i < NUM_REG; i++)
		vm->frame[vm->fp][i].type = VAL_UNDEF;

	vm->module = oak_realloc(vm->module, (vm->fp + 1) * sizeof *vm->module);
	vm->module[vm->fp] = vm->m->id;

	vm->maxfp = vm->fp > vm->maxfp ? vm->fp : vm->maxfp;
}

struct vm *
new_vm(struct module *m, struct oak *k, bool debug)
{
	struct vm *vm = oak_malloc(sizeof *vm);

	memset(vm, 0, sizeof *vm);
	vm->r = new_reporter();

	vm->code  = m->code;
	vm->debug = debug;
	vm->ct    = m->ct;
	vm->gc    = m->gc;
	vm->f     = stderr;
	vm->k     = k;
	vm->m     = m;

	return vm;
}

void
free_vm(struct vm *vm)
{
	if (!vm) return;

	for (size_t i = 1; i < vm->maxfp + 1; i++)
		free(vm->frame[i]);

	error_clear(vm->r);

	free(vm->frame);
	free(vm->stack);
	free(vm->callstack);
	free(vm->imp);
	free(vm->subject);
	free(vm->module);

	free(vm);
}

static void
pop_frame(struct vm *vm)
{
	vm->fp--;
}

static void
push(struct vm *vm, struct value v)
{
	if (vm->debug) {
		fprintf(stderr, "pushing value ");
		print_debug(vm->gc, v);
		fprintf(stderr, " in module `%s'\n", vm->m->name);
	}

	vm->sp++;

	if (vm->sp <= vm->maxsp) {
		vm->stack[vm->sp] = v;
		return;
	}

	vm->stack = oak_realloc(vm->stack, (vm->sp + 1) * sizeof *vm->stack);
	vm->stack[vm->sp] = v;
	vm->maxsp = vm->sp > vm->maxsp ? vm->sp : vm->maxsp;
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
		fprintf(stderr, "<function call : %s@%s : %p : %zu argument%s>\n",
		        v.name ? v.name : "*function*",
		        vm->k->modules[v.module]->name,
		        (void *)&vm->code[vm->ip], vm->sp,
		        vm->sp == 1 ? "" : "s");

	if (v.module != vm->m->id) {
		struct module *m = vm->k->modules[v.module];

		if (m->vm != vm) {
			for (size_t i = 1; i <= vm->sp; i++)
				push(m->vm, value_translate(m->gc, vm->gc, vm->stack[i]));
			vm->sp = 0;
		}

		if (m->vm == vm) {
			struct instruction *c = vm->code;
			struct module *m2 = vm->m;
			int ip = vm->ip;
			struct constant_table *ct = vm->ct;

			vm->ct = m->ct;
			vm->code = m->code;
			vm->m = m;
			m->vm = vm;

			push_frame(m->vm);
			execute(m->vm, v.integer);

			vm->code = c;
			vm->m = m2;
			vm->ip = ip;
			vm->ct = ct;

			if (m->vm->sp) push(vm, value_translate(vm->gc, m->gc, m->vm->stack[m->vm->sp--]));
			else push(vm, NIL);
			return;
		}

		push_frame(m->vm);
		execute(m->vm, v.integer);
		if (m->vm->sp) push(vm, value_translate(vm->gc, m->gc, m->vm->stack[m->vm->sp--]));
		else push(vm, NIL);
		return;
	}

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
	if (vm->fp == 1 || (vm->module[vm->fp] != vm->module[vm->fp - 1])) {
		if (vm->debug)
			DOUT("returning from call as a module");

		vm->returning = true;
		pop_frame(vm);
		return;
	}

	assert(vm->fp > 0);

	pop_frame(vm);
	if (vm->csp == 0) return;
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
		fprintf(stderr, " @%"PRIu64" ", vm->calls[i].integer);
		fprintf(stderr, "%s:%zu:%zu\n",
		        c.loc->file, line_number(*c.loc), column_number(*c.loc));
	}

	if (depth != 0)
		fprintf(stderr, "\t--- Truncated ---\n");
}

#define GETREG(X) ((X) >= NUM_REG ? vm->frame[1][(X) - NUM_REG] : vm->frame[vm->fp][X])
/* #define SETREG(X,Y) ((X) >= NUM_REG ? (vm->frame[1][(X) - NUM_REG] = (Y)) : (vm->frame[vm->fp][X] = (Y))) */

#define SETREG(X,Y)	  \
	do { \
		struct value _ = (Y); \
		if (_.type == VAL_ERR) \
			error_push(vm->r, *vm->code[vm->ip].loc, ERR_FATAL, "ValueError: %s", _.name); \
		((X) >= NUM_REG ? (vm->frame[1][(X) - NUM_REG] = (_)) : (vm->frame[vm->fp][X] = (_))); \
	} while (0)

#define SETR(X,Y,Z) ((X) >= NUM_REG ? (vm->frame[1][(X) - NUM_REG].Y = (Z)) : (vm->frame[vm->fp][X].Y = (Z)))

#define CONST(X) (vm->ct->val[X])
#define BIN(X) SETREG(c.a, X##_values(vm->gc, GETREG(c.b), GETREG(c.c)))

static void
pop(struct vm *vm, int reg)
{
	if (vm->sp)
		SETREG(reg, vm->stack[vm->sp--]);
}

static int
find_undef(struct vm *vm)
{
	int i = NUM_REG;
	while (i && GETREG(i - 1).type == VAL_UNDEF) i--;

	/* TODO: error */
	return i;
}

static struct value
eval(struct vm *vm, char *s, int scope, struct location loc, int stack_base)
{
	if (vm->debug) fprintf(stderr, "<evaluating '%s'>\n", s);
	struct module *m = load_module(vm->k,
	                               find_from_scope(vm->m->sym, scope),
	                               strclone(s),
	                               "*eval.k*",
	                               "*eval*",
	                               vm, stack_base);

	if (vm->debug) DOUT("finished evaluation");
	if (!m) {
		error_push(vm->r, loc, ERR_FATAL, "eval failed");
		return NIL;
	}

	struct value v;
	vm->module[vm->fp] = vm->m->id;
	v = value_translate(vm->gc, m->gc, vm->k->stack[--vm->k->sp]);

	if (vm->returning)
		vm->ip = vm->callstack[vm->csp--];

	vm->returning = false;

	for (int i = stack_base; i < NUM_REG; i++)
		SETR(i, type, VAL_UNDEF);

	return v;
}

static char *parse_interpolation(struct vm *vm, char *input, int *len)
{
	char *e = strclone("");

	struct location loc = *vm->code[vm->ip].loc;
	loc.len = 1;
	loc.index += *len + 1;

	if (*input == '$' && isdigit(input[1])) {
		int i = 1;

		while (input[i] && isdigit(input[i])) i++;

		e = oak_realloc(e, i + 1);
		strncpy(e, input, i);
		e[i] = 0;
		(*len) += i - 1;
		return e;
	} else if (*input == '$') {
		input++;

		int i = 0;
		while (input[i] && isalnum(input[i])) i++;

		e = oak_realloc(e, i + 1);
		strncpy(e, input, i);
		e[i] = 0;
		(*len) += i;
		return e;
	}

	if (*input == '{') input++;

	int i;
	for (i = 0; i < (int)strlen(input); i++) {
		(*len)++;

		if (input[i] == '}' && input[i + 1] == '}') {
			(*len)++;
			i++;
			e = smart_cat(e, "}");
		} else if (input[i] == '}') {
			break;
		} else {
			e = oak_realloc(e, strlen(e) + 2);
			append_char(e, input[i]);
		}
	}

	if (!input[i] || !i)
		error_push(vm->r, loc, ERR_FATAL, "invalid interpolation; no matching '}'");

	return e;
}

static inline void
execute_instr(struct vm *vm, struct instruction c)
{
	switch (c.type) {
	case INSTR_MOV:  SETREG(c.a, GETREG(c.b));              break;
	case INSTR_JMP:  vm->ip = c.a - 1;                      break;
	case INSTR_PUSH: push(vm, GETREG(c.a));                 break;
	case INSTR_POP:  pop(vm, c.a);                          break;
	case INSTR_CALL: call(vm, GETREG(c.a));                 break;
	case INSTR_RET:  ret(vm);                               break;
	case INSTR_ADD:  BIN(add);                              break;
	case INSTR_SUB:  BIN(sub);                              break;
	case INSTR_MUL:  BIN(mul);                              break;
	case INSTR_POW:  BIN(pow);                              break;
	case INSTR_DIV:  BIN(div);                              break;
	case INSTR_MOD:  BIN(mod);                              break;
	case INSTR_CMP:  BIN(cmp);                              break;
	case INSTR_LESS: BIN(less);                             break;
	case INSTR_LEQ:  BIN(leq);                              break;
	case INSTR_GEQ:  BIN(geq);                              break;
	case INSTR_BAND: BIN(band);                             break;
	case INSTR_XOR:  BIN(xor);                              break;
	case INSTR_BOR:  BIN(bor);                              break;
	case INSTR_MORE: BIN(more);                             break;
	case INSTR_SLEFT: BIN(sleft);                           break;
	case INSTR_SRIGHT: BIN(sright);                         break;
	case INSTR_INC: SETREG(c.a, inc_value(GETREG(c.a)));    break;
	case INSTR_DEC: SETREG(c.a, dec_value(GETREG(c.a)));    break;
	case INSTR_MSET: vm->match = c.a;                       break;
	case INSTR_MINC:
		if (vm->match == 65535) vm->match = 0;
		else vm->match++;
		break;

	case INSTR_LINE:
		if (!vm->debug && vm->k->talkative) fputc('\n', vm->f);
		break;

	case INSTR_CHKSTCK:
		/* if (vm->sp) */
		/* 	error_push(vm->r, *c.loc, ERR_FATAL, "invalid number of arguments passed to function (received %d too many)", vm->sp); */
		break;

	case INSTR_FLIP:
		SETREG(c.a, flip_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_NEG:
		SETREG(c.a, neg_value(GETREG(c.b)));
		break;

	case INSTR_COPY:
		SETREG(c.a, copy_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_COPYC:
		SETREG(c.a, copy_value(vm->gc, CONST(c.b)));
		break;

	case INSTR_INT:
		if (GETREG(c.b).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "int builtin requires string argument (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		SETREG(c.a, int_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_POPALL: {
		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[v.idx] = new_array();

		for (size_t i = vm->sp; i > 0; i--)
			array_push(vm->gc->array[v.idx], vm->stack[i]);

		SETREG(c.a, v);
		vm->sp = 0;
	} break;

	case INSTR_FLOAT:
		if (GETREG(c.b).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "float builtin requires string argument (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		SETREG(c.a, float_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_STR: {
		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = show_value(vm->gc, GETREG(c.b));
		SETREG(c.a, v);
	} break;

	case INSTR_SUBSCR:
		if (GETREG(c.b).type == VAL_ARRAY) {
			if (GETREG(c.c).type != VAL_INT) {
				error_push(vm->r, *c.loc, ERR_FATAL,
				           "array requires integer subscript (got %s)", value_data[GETREG(c.c).type].body);
				return;
			}

			if (GETREG(c.c).integer >= vm->gc->array[GETREG(c.b).idx]->len
			    || GETREG(c.c).integer < 0) {
				struct value v;
				v.type = VAL_NIL;
				SETREG(c.a, v);
				break;
			}

			SETREG(c.a, vm->gc->array[GETREG(c.b).idx]->v
			       [GETREG(c.c).integer]);
		} else if (GETREG(c.b).type == VAL_TABLE) {
			if (GETREG(c.c).type != VAL_STR) {
				error_push(vm->r, *c.loc, ERR_FATAL,
				           "table requires string subscript (got %s)", value_data[GETREG(c.c).type].body);
				return;
			}

			SETREG(c.a,
			       table_lookup(vm->gc->table[GETREG(c.b).idx],
			                    vm->gc->str[GETREG(c.c).idx]));
		} else if (GETREG(c.b).type == VAL_STR) {
			if (GETREG(c.c).type != VAL_INT) {
				error_push(vm->r, *c.loc, ERR_FATAL,
				           "string requires integer subscript (got %s)", value_data[GETREG(c.c).type].body);
				return;
			}

			if ((size_t)GETREG(c.c).integer <= strlen(vm->gc->str[GETREG(c.b).idx])) {
				struct value v;
				v.type = VAL_STR;
				v.idx = gc_alloc(vm->gc, VAL_STR);
				vm->gc->str[v.idx] = strclone(" ");
				vm->gc->str[v.idx][0] = vm->gc->str[GETREG(c.b).idx][GETREG(c.c).integer];
				SETREG(c.a, v);
			} else {
				SETREG(c.a, ((struct value){ VAL_NIL, { 0 }, 0}));
			}
		} else {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "invalid subscript on unsubscriptable type %s", value_data[GETREG(c.b).type].body);
		}
		break;

#define CHECKREG(X,Y,...)	  \
		if (Y) { \
			error_push(vm->r, *c.loc, ERR_FATAL, __VA_ARGS__); \
			return; \
		}

	case INSTR_SLICE: {
		CHECKREG(c.b, GETREG(c.b).type != VAL_ARRAY,
		         "array slice requires array operand (got %s)",
		         value_data[GETREG(c.b).type].body);

		CHECKREG(c.c, GETREG(c.c).type != VAL_INT
		         && GETREG(c.c).type != VAL_NIL,
		         "array slice requires integer startpoint (got %s)",
		         value_data[GETREG(c.c).type].body);

		CHECKREG(c.d, GETREG(c.d).type != VAL_INT
		         && GETREG(c.d).type != VAL_NIL,
		         "array slice requires integer endpoint (got %s)",
		         value_data[GETREG(c.d).type].body);

		CHECKREG(c.e, GETREG(c.e).type != VAL_INT
		         && GETREG(c.e).type != VAL_NIL,
		         "array slice requires integer step (got %s)",
		         value_data[GETREG(c.e).type].body);

		int64_t start = GETREG(c.c).type == VAL_INT ? GETREG(c.c).integer : 0;
		int64_t stop = GETREG(c.d).type == VAL_INT ? GETREG(c.d).integer : vm->gc->array[GETREG(c.b).idx]->len - 1;
		int64_t step = GETREG(c.e).type == VAL_INT ? GETREG(c.e).integer : 1;

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[v.idx] = new_array();

		if (stop < start && step < 0)
			for (int64_t i = start; i >= stop; i += step)
				array_push(vm->gc->array[v.idx], vm->gc->array[GETREG(c.b).idx]->v[labs(i % vm->gc->array[GETREG(c.b).idx]->len)]);
		else if (start < stop && step < 0)
			for (int64_t i = stop; i >= start; i += step)
				array_push(vm->gc->array[v.idx], vm->gc->array[GETREG(c.b).idx]->v[labs(i % vm->gc->array[GETREG(c.b).idx]->len)]);
		else if (start < stop && step > 0)
			for (int64_t i = start; i <= stop; i += step)
				array_push(vm->gc->array[v.idx], vm->gc->array[GETREG(c.b).idx]->v[labs(i % vm->gc->array[GETREG(c.b).idx]->len)]);

		SETREG(c.a, v);
	} break;

	case INSTR_PUSHBACK:
		array_push(vm->gc->array[GETREG(c.a).idx],
		           GETREG(c.b));
		break;

	case INSTR_ASET:
		if (GETREG(c.a).type == VAL_STR
		    && GETREG(c.b).type == VAL_INT
		    && ((size_t)GETREG(c.b).integer
		        <= strlen(vm->gc->str[GETREG(c.a).idx]))) {

			struct value v = GETREG(c.a);
			char *a = show_value(vm->gc, GETREG(c.c));
			char *b = strclone(vm->gc->str[v.idx]);

			vm->gc->str[v.idx] = oak_realloc(vm->gc->str[v.idx],
			                                 strlen(b)
			                                 + strlen(a) + 1);

			vm->gc->str[v.idx][GETREG(c.b).integer] = 0;
			strcat(vm->gc->str[v.idx], a);
			strcat(vm->gc->str[v.idx], b + GETREG(c.b).integer + 1);

			free(a);
			free(b);
			return;
		} else if (GETREG(c.a).type == VAL_STR
		           && GETREG(c.b).type == VAL_INT) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "invalid index into string of length %zu",
			           strlen(vm->gc->str[GETREG(c.a).idx]));
			return;
		}

		if (GETREG(c.a).type != VAL_ARRAY
		    && GETREG(c.b).type == VAL_INT) {
			SETR(c.a, type, VAL_ARRAY);
			SETR(c.a, idx, gc_alloc(vm->gc, VAL_ARRAY));
			vm->gc->array[GETREG(c.a).idx] = new_array();
		}

		if (GETREG(c.a).type != VAL_TABLE
		    && GETREG(c.b).type == VAL_STR) {
			SETR(c.a, type, VAL_TABLE);
			SETR(c.a, idx, gc_alloc(vm->gc, VAL_TABLE));
			vm->gc->table[GETREG(c.a).idx] = new_table();
		}

		if (GETREG(c.a).type == VAL_ARRAY
		    && GETREG(c.b).type == VAL_INT) {
			grow_array(vm->gc->array[GETREG(c.a).idx], GETREG(c.b).integer + 1);

			if (vm->gc->array[GETREG(c.a).idx]->len <= GETREG(c.b).integer) {
				vm->gc->array[GETREG(c.a).idx]->len = GETREG(c.b).integer + 1;
			}

			vm->gc->array[GETREG(c.a).idx]->v[GETREG(c.b).integer]
				= GETREG(c.c);
		}

		if (GETREG(c.b).type == VAL_STR) {
			table_add(vm->gc->table[GETREG(c.a).idx],
			          vm->gc->str[GETREG(c.b).idx],
			          GETREG(c.c));
		}
		break;

	case INSTR_APUSH: {
		if (GETREG(c.a).type != VAL_ARRAY) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "push builtin requires array as its lefthand argument (got %s)",
			           value_data[GETREG(c.a).type].body);
			return;
		}

		array_push(vm->gc->array[GETREG(c.a).idx], copy_value(vm->gc, GETREG(c.b)));
	} break;

	case INSTR_INS: {
		if (GETREG(c.a).type != VAL_ARRAY) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "insert builtin requires array as its lefthand argument (got %s)",
			           value_data[GETREG(c.a).type].body);
			return;
		}

		if (GETREG(c.b).type != VAL_INT) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "insert builtin requires integer as its index argument (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		array_insert(vm->gc->array[GETREG(c.a).idx], GETREG(c.b).integer, copy_value(vm->gc, GETREG(c.c)));
	} break;

	case INSTR_DEREF:
		/* TODO: TABLES! */
		if (GETREG(c.b).type != VAL_ARRAY) {
			SETREG(c.a, ((struct value){ VAL_NIL, { 0 }, 0}));
			return;
		}

		grow_array(vm->gc->array[GETREG(c.b).idx], GETREG(c.c).integer + 1);

		if (GETREG(c.c).integer < vm->gc->array[GETREG(c.b).idx]->len &&
		    vm->gc->array[GETREG(c.b).idx]->v[GETREG(c.c).integer].type == VAL_ARRAY) {
			SETREG(c.a, vm->gc->array[GETREG(c.b).idx]->v
			       [GETREG(c.c).integer]);
		} else {
			struct value v;
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->array[v.idx] = new_array();

			grow_array(vm->gc->array[GETREG(c.b).idx], GETREG(c.c).integer + 1);
			vm->gc->array[GETREG(c.b).idx]->len = GETREG(c.c).integer + 1;
			vm->gc->array[GETREG(c.b).idx]->v[GETREG(c.c).integer] = v;

			SETREG(c.a, v);
		}
		break;

	case INSTR_PRINT:
		if (vm->k->talkative) print_value(vm->f, vm->gc, GETREG(c.a));
		if (vm->debug && vm->k->talkative) fputc('\n', stderr);
		break;

	case INSTR_COND:
		if (is_truthy(vm->gc, GETREG(c.a)))
			vm->ip++;
		break;

	case INSTR_NCOND:
		if (!is_truthy(vm->gc, GETREG(c.a)))
			vm->ip++;
		break;

	case INSTR_TYPE: {
		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);

		vm->gc->str[v.idx] = oak_malloc(strlen(value_data[GETREG(c.b).type].body) + 1);
		strcpy(vm->gc->str[v.idx], value_data[GETREG(c.b).type].body);

		SETREG(c.a, v);
	} break;

	case INSTR_LEN:
		SETREG(c.a, value_len(vm->gc, GETREG(c.b)));
		break;

	case INSTR_KILL:
		assert(GETREG(c.a).type == VAL_STR);
		error_push(vm->r, *c.loc, ERR_KILLED, vm->gc->str[GETREG(c.a).idx]);
		break;

	case INSTR_PUSHIMP:
		vm->imp = oak_realloc(vm->imp, (vm->impp + 1) * sizeof *vm->imp);
		vm->imp[vm->impp++] = GETREG(c.a);
		break;

	case INSTR_POPIMP:
		assert(vm->impp);
		vm->impp--;
		break;

	case INSTR_GETIMP:
		if (vm->impp) SETREG(c.a, vm->imp[vm->impp - 1]);
		else SETREG(c.a, NIL);
		break;

	case INSTR_RESETR:
		assert(GETREG(c.a).type == VAL_REGEX);
		vm->gc->regex[GETREG(c.a).idx]->cont = 0;
		break;

	case INSTR_MATCH: {
		if (GETREG(c.c).type != VAL_REGEX) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "attempt to apply match to non regular expression value (got %s)",
			           value_data[GETREG(c.c).type].body);
			return;
		}

		if (GETREG(c.b).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "attempt to apply regular expression to non-string value (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		free(vm->subject);
		vm->subject = NULL;

		int **vec = NULL;
		struct ktre *re = vm->gc->regex[GETREG(c.c).idx];
		char *subject = vm->gc->str[GETREG(c.b).idx];
		bool ret = ktre_exec(re, subject, &vec);

		vm->subject = strclone(subject);
		vm->re = re;

		SETR(c.a, type, VAL_ARRAY);
		SETR(c.a, idx, gc_alloc(vm->gc, VAL_ARRAY));
		vm->gc->array[GETREG(c.a).idx] = new_array();

		if (ret) {
			for (int i = 0; i < re->num_matches; i++) {
				struct value v;
				v.type = VAL_STR;
				v.idx = gc_alloc(vm->gc, VAL_STR);
				vm->gc->str[v.idx] = oak_malloc(vec[i][1] + 1);
				strncpy(vm->gc->str[v.idx], subject + vec[i][0], vec[i][1]);
				vm->gc->str[v.idx][vec[i][1]] = 0;
				array_push(vm->gc->array[GETREG(c.a).idx], v);
			}
		} else if (re->err) {
			struct location loc = *c.loc;
			loc.len = 1;
			loc.index += re->loc;

			error_push(vm->r, loc, ERR_FATAL,
			           "regex failed at runtime with error code %d: %s",
			           re->err,
			           re->err_str ? re->err_str : "no error message");
			return;
		}

		vm->match = re->num_matches - 1;
	} break;

	case INSTR_SUBST: {
		assert(GETREG(c.a).type == VAL_STR);
		assert(GETREG(c.b).type == VAL_REGEX);
		assert(GETREG(c.c).type == VAL_STR);

		free(vm->subject);
		vm->subject = NULL;

		struct ktre *re = vm->gc->regex[GETREG(c.b).idx];
		char *subject = strclone(vm->gc->str[GETREG(c.a).idx]);
		char *subst = vm->gc->str[GETREG(c.c).idx];

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = NULL;

		if (GETREG(c.b).e == 0) {
			char *ret = ktre_filter(re, subject, subst, "$");

			if (re->err) {
				error_push(vm->r, *c.loc, ERR_FATAL, "regex failed at runtime with %d: %s", re->err, re->err_str ? re->err_str : "no message");
				return;
				/* fprintf(stderr, "\t%s\n\t", re->pat); */
				/* for (int i = 0; i < re->loc; i++) fprintf(stderr, " "); */
				/* fprintf(stderr, "^"); */
			} else if (ret) {
				vm->gc->str[v.idx] = ret;
			} else {
				vm->gc->str[v.idx] = strclone(subject);
			}
		} else {
			int **vec = NULL;
			ktre_exec(re, subject, &vec);

			if (!re->num_matches) {
				v.type = VAL_NIL;
				SETREG(c.a, v);
				return;
			}

			char *a = oak_malloc(128);
			strncpy(a, subject, vec[0][0]);
			a[vec[0][0]] = 0;

			for (int i = 0; i < re->num_matches; i++) {
				char *s = strclone(subst);
				vm->match = i;

				for (int j = 0; j < GETREG(c.b).e; j++) {
					free(vm->subject);
					vm->re = re;
					vm->subject = strclone(subject);

					SETREG(c.c, eval(vm, s, c.d,
					                 *c.loc, find_undef(vm)));

					if (vm->r->pending) {
						free(vm->subject);
						vm->subject = NULL;
						free(subject);
						free(a);
						return;
					}

					vm->re = re;
					free(vm->subject);
					vm->subject = strclone(subject);

					free(s);
					s = show_value(vm->gc, GETREG(c.c));
				}

				a = oak_realloc(a, strlen(a) + strlen(s) + 1);
				strcat(a, s);

				if (i != re->num_matches - 1) {
					a = oak_realloc(a, strlen(a) + (vec[i + 1][0] - (vec[i][0] + vec[i][1])) + 1);
					strncat(a, subject + vec[i][0] + vec[i][1],
					        vec[i + 1][0] - (vec[i][0] + vec[i][1]));
				}

				free(s);
			}

			a = oak_realloc(a, strlen(a) + (strlen(subject) - (vec[re->num_matches - 1][0] + vec[re->num_matches - 1][1])) + 1);
			strcat(a, subject + vec[re->num_matches - 1][0]
			       + vec[re->num_matches - 1][1]);

			vm->gc->str[v.idx] = a;
		}

		free(subject);
		vm->re = re;
		SETREG(c.a, v);
	} break;

	case INSTR_GROUP: {
		assert(GETREG(c.b).type == VAL_INT);

		if (!vm->re) {
			SETR(c.a, type, VAL_NIL);
			return;
		}

		if (GETREG(c.b).integer >= vm->re->num_groups) {
			error_push(vm->r, *c.loc, ERR_FATAL, "group does not exist");
			return;
		}

		int m = vm->match;
		int **vec = ktre_getvec(vm->re);

		/* TODO: check that it's not beyond the end of the thing */
		if (m < 0) {
			for (int i = 0; i < vm->re->num_matches; i++)
				free(vec[i]);

			free(vec);
			SETR(c.a, type, VAL_NIL);
			return;
		}

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);

		vm->gc->str[v.idx] = oak_malloc(vec[m][GETREG(c.b).integer * 2 + 1] + 1);
		strncpy(vm->gc->str[v.idx], vm->subject + vec[m][GETREG(c.b).integer * 2], vec[m][GETREG(c.b).integer * 2 + 1]);
		vm->gc->str[v.idx][vec[m][GETREG(c.b).integer * 2 + 1]] = 0;

		for (int i = 0; i < vm->re->num_matches; i++)
			free(vec[i]);

		free(vec);
		SETREG(c.a, v);
	} break;

	case INSTR_SPLIT: {
		int len = 0;
		ktre *re = vm->gc->regex[GETREG(c.c).idx];
		char *subject = vm->gc->str[GETREG(c.b).idx];
		char **split = ktre_split(re, subject, &len);

		SETR(c.a, type, VAL_ARRAY);
		SETR(c.a, idx, gc_alloc(vm->gc, VAL_ARRAY));
		vm->gc->array[GETREG(c.a).idx] = new_array();

		for (int i = 0; i < len; i++) {
			struct value v;
			v.type = VAL_STR;
			v.idx = gc_alloc(vm->gc, VAL_STR);
			vm->gc->str[v.idx] = oak_malloc(strlen(split[i]) + 1);
			strcpy(vm->gc->str[v.idx], split[i]);
			array_push(vm->gc->array[GETREG(c.a).idx], v);

			free(split[i]);
		}

		free(split);
	} break;

	case INSTR_JOIN: {
		if (GETREG(c.b).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "join builtin requires string lefthand argument (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		if (GETREG(c.c).type != VAL_ARRAY) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "join builtin requires array righthand argument (got %s)",
			           value_data[GETREG(c.c).type].body);
			return;
		}

		if (vm->gc->array[GETREG(c.c).idx]->len == 0) {
			struct value v;
			v.type = VAL_STR;
			v.idx = gc_alloc(vm->gc, VAL_STR);
			vm->gc->str[v.idx] = strclone("");
			SETREG(c.a, v);
			return;
		}

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = NULL;

		char *a = strclone("");
		size_t arrlen = vm->gc->array[GETREG(c.c).idx]->len;

		for (size_t i = 0; i < arrlen - 1; i++) {
			char *b = show_value(vm->gc, vm->gc->array[GETREG(c.c).idx]->v[i]);
			a = oak_realloc(a, strlen(a) + strlen(b)
			                + strlen(vm->gc->str[GETREG(c.b).idx]) + 1);
			strcat(a, b);
			strcat(a, vm->gc->str[GETREG(c.b).idx]);
			free(b);
		}

		char *b = show_value(vm->gc, vm->gc->array[GETREG(c.c).idx]->v[arrlen - 1]);
		a = oak_realloc(a, strlen(a) + strlen(b) + 1);
		strcat(a, b);
		free(b);

		vm->gc->str[v.idx] = a;
		SETREG(c.a, v);
	} break;

	case INSTR_RANGE: {
		assert(GETREG(c.b).type == VAL_INT || GETREG(c.b).type == VAL_FLOAT);
		assert(GETREG(c.c).type == VAL_INT
		       || GETREG(c.c).type == VAL_FLOAT
		       || GETREG(c.c).type == VAL_ARRAY
		       || GETREG(c.c).type == VAL_STR);
		assert(GETREG(c.d).type == VAL_INT || GETREG(c.d).type == VAL_FLOAT);

		double start = GETREG(c.b).type == VAL_INT ? (double)GETREG(c.b).integer : GETREG(c.b).real;
		double stop = -1;
		double step = GETREG(c.d).type == VAL_INT ? (double)GETREG(c.d).integer : GETREG(c.d).real;

		if (GETREG(c.c).type == VAL_ARRAY) {
			stop = (double)vm->gc->array[GETREG(c.c).idx]->len - 1;
		} else if (GETREG(c.c).type == VAL_STR) {
			stop = strlen(vm->gc->str[GETREG(c.c).idx]);
		} else {
			stop = GETREG(c.c).type == VAL_INT ? (double)GETREG(c.c).integer : GETREG(c.c).real;
		}

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[v.idx] = new_array();

		if (fcmp(start, stop)) {
			array_push(vm->gc->array[v.idx], GETREG(c.b));
			SETREG(c.a, v);
			return;
		}

		if (start < stop) {
			if (step <= 0) {
				error_push(vm->r, *c.loc, ERR_FATAL,
				           "invalid range; range from %f to %f requires a positive step value (got %f)",
				           start, stop, step);
				return;
			}

			grow_array(vm->gc->array[v.idx], (stop - start) / step + 1);
			for (double i = start; i <= stop; i += step) {
				if (GETREG(c.b).type == VAL_FLOAT || GETREG(c.c).type == VAL_FLOAT)
					array_push(vm->gc->array[v.idx], (struct value){ VAL_FLOAT, { .real = i }, 0 });
				else
					array_push(vm->gc->array[v.idx], (struct value){ VAL_INT, { .integer = (int64_t)i }, 0 });
			}
		} else {
			if (step >= 0) {
				error_push(vm->r, *c.loc, ERR_FATAL,
				           "invalid range; range from %f to %f requires a negative step value (got %f)",
				           start, stop, step);
				return;
			}

			grow_array(vm->gc->array[v.idx], (start - stop) / step + 1);
			for (double i = start; i >= stop; i += step) {
				if (GETREG(c.b).type == VAL_FLOAT || GETREG(c.c).type == VAL_FLOAT)
					array_push(vm->gc->array[v.idx], (struct value){ VAL_FLOAT, { .real = i }, 0 });
				else
					array_push(vm->gc->array[v.idx], (struct value){ VAL_INT, { .integer = i }, 0 });
			}
		}

		SETREG(c.a, v);
	} break;

	case INSTR_REV:
		SETREG(c.a, rev_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_SORT:
		SETREG(c.a, sort_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_SUM: {
		if (GETREG(c.b).type != VAL_ARRAY) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "sum builtin requires array operand (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		struct value v;
		v.type = VAL_FLOAT;
		v.real = 0;

		for (size_t i = 0; i < vm->gc->array[GETREG(c.b).idx]->len; i++) {
			if (vm->gc->array[GETREG(c.b).idx]->v[i].type == VAL_INT)
				v.real += vm->gc->array[GETREG(c.b).idx]->v[i].integer;

			if (vm->gc->array[GETREG(c.b).idx]->v[i].type == VAL_FLOAT)
				v.real += vm->gc->array[GETREG(c.b).idx]->v[i].real;
		}

		SETREG(c.a, v);
	} break;

	case INSTR_COUNT: {
		if (GETREG(c.b).type != VAL_ARRAY) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "count builtin requires array operand (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		struct value v;
		v.type = VAL_INT;
		v.integer = 0;

		for (size_t i = 0; i < vm->gc->array[GETREG(c.b).idx]->len; i++) {
			if (is_truthy(vm->gc,
			              cmp_values(vm->gc,
			                         vm->gc->array[GETREG(c.b).idx]->v[i],
			                         GETREG(c.c))))
				v.integer++;
		}

		SETREG(c.a, v);
	} break;

	case INSTR_ABS: {
		if (GETREG(c.b).type != VAL_INT
		    && GETREG(c.b).type != VAL_FLOAT) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "abs builtin requires numeric operand (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		SETREG(c.a, abs_value(GETREG(c.b)));
	} break;

	case INSTR_INTERP: {
		assert(GETREG(c.b).type == VAL_STR);

		if (!strchr(vm->gc->str[GETREG(c.b).idx], '{')
		    && !strchr(vm->gc->str[GETREG(c.b).idx], '$')) {
			SETREG(c.a, GETREG(c.b));
			return;
		}

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = NULL;

		char *s = oak_malloc(strlen(vm->gc->str[GETREG(c.b).idx]) + 1);
		*s = 0;

		char *a = vm->gc->str[GETREG(c.b).idx];

		for (int i = 0; i < (int)strlen(a); i++) {
			if (a[i] == '\\' && a[i + 1] == '$') {
				i++;
				s = oak_realloc(s, strlen(s) + 2);
				strncat(s, a + 1, 1);
				continue;
			} else if (a[i] == '\\') {
				i++;
				s = oak_realloc(s, strlen(s) + 3);
				strncat(s, a, 2);
				continue;
			}

			if (a[i] == '{' && a[i + 1] == '{') {
				i++;
				s = smart_cat(s, "{");
			} else if (a[i] == '{' || (a[i] == '$' && isalnum(a[i + 1]))) {
				char *e = parse_interpolation(vm, a + i, &i);

				if (vm->r->pending) {
					free(s);
					free(e);
					return;
				}

				char *sv = show_value(vm->gc, eval(vm, e, c.c, *c.loc, find_undef(vm)));
				s = smart_cat(s, sv);
				free(e);
				free(sv);
			} else if (a[i] == '}' && a[i + 1] == '}') {
				i++;
				s = smart_cat(s, "}");
			} else {
				s = oak_realloc(s, strlen(s) + 2);
				append_char(s, a[i]);
			}
		}

		vm->gc->str[v.idx] = s;
		SETREG(c.a, v);
	} break;

	case INSTR_EVAL:
		if (GETREG(c.b).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "eval requires string argument (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		SETREG(c.a, eval(vm, vm->gc->str[GETREG(c.b).idx],
		                 GETREG(c.c).integer, *c.loc, find_undef(vm)));
		break;

	case INSTR_VALUES:
		if (GETREG(c.b).type != VAL_TABLE) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "values builtin requires table argument (got %s)",
			           value_data[GETREG(c.b).type].body);
			return;
		}

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[v.idx] = new_array();

		for (int i = 0; i < TABLE_SIZE; i++) {
			struct bucket b = vm->gc->table[GETREG(c.b).idx]->bucket[i];
			for (size_t j = 0; j < b.len; j++) {
				array_push(vm->gc->array[v.idx], b.val[j]);
			}
		}

		SETREG(c.a, v);
		break;

	case INSTR_NOP:
		break;

	case INSTR_ESCAPE:
		/* Fake a function call return */
		vm->returning = true;
		vm->escaping = true;
		vm->callstack = oak_realloc(vm->callstack,
		                            (vm->csp + 2) * sizeof *vm->callstack);
		vm->callstack[++vm->csp] = c.a - 1;
		break;

	case INSTR_UC:
		assert(GETREG(c.b).type == VAL_STR);
		SETREG(c.a, uc_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_LC:
		assert(GETREG(c.b).type == VAL_STR);
		SETREG(c.a, lc_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_UCFIRST:
		assert(GETREG(c.b).type == VAL_STR);
		SETREG(c.a, ucfirst_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_LCFIRST:
		assert(GETREG(c.b).type == VAL_STR);
		SETREG(c.a, lcfirst_value(vm->gc, GETREG(c.b)));
		break;

	case INSTR_CHR: {
		struct array *a = NULL;
		assert(vm->sp);

		if (vm->sp == 1) {
			struct value t = vm->stack[vm->sp--];
			if (t.type == VAL_ARRAY)
				a = vm->gc->array[t.idx];
			else {
				a = new_array();
				vm->gc->array[gc_alloc(vm->gc, VAL_ARRAY)] = a;
				array_push(a, t);
			}
		} else {
			struct value t;
			t.type = VAL_ARRAY;
			t.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->array[t.idx] = new_array();

			while (vm->sp)
				array_push(vm->gc->array[t.idx], vm->stack[vm->sp--]);

			a = vm->gc->array[t.idx];
		}

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = NULL;
		vm->gc->str[v.idx] = oak_malloc(a->len + 1);
		*vm->gc->str[v.idx] = 0;

		for (size_t i = 0; i < a->len; i++)
			if (a->v[i].type == VAL_INT)
				append_char(vm->gc->str[v.idx], a->v[i].integer);

		SETREG(c.a, v);
	} break;

	case INSTR_ORD: {
		assert(vm->sp);
		struct value s;

		if (vm->sp == 1) {
			s = vm->stack[vm->sp--];
		} else {
			s.type = VAL_ARRAY;
			s.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->array[s.idx] = new_array();

			while (vm->sp)
				array_push(vm->gc->array[s.idx], vm->stack[vm->sp--]);
		}

		if (s.type != VAL_ARRAY && s.type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "ord builtin requires a string or array argument (got %s)",
			           value_data[s.type].body);
			return;
		}

		if (s.type == VAL_STR && strlen(vm->gc->str[s.idx]) == 1) {
			SETREG(c.a, INT(*vm->gc->str[s.idx]));
			return;
		}

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[v.idx] = new_array();
		struct array *a = vm->gc->array[v.idx];

		if (s.type == VAL_STR) {
			char *str = vm->gc->str[s.idx];
			for (size_t i = 0; i < strlen(str); i++)
				array_push(a, INT(str[i]));
		} else {
			for (size_t i = 0; i < vm->gc->array[s.idx]->len; i++) {
				if (vm->gc->array[s.idx]->v[i].type != VAL_STR) continue;

				char *str = vm->gc->str[vm->gc->array[s.idx]->v[i].idx];
				for (size_t j = 0; j < strlen(str); j++)
					array_push(a, INT(str[j]));
			}
		}

		SETREG(c.a, v);
	} break;

	case INSTR_MIN:
		if (vm->sp == 1) {
			SETREG(c.a, min_value(vm->gc, vm->stack[vm->sp--]));
			return;
		} else {
			struct value v;
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->array[v.idx] = new_array();

			while (vm->sp)
				array_push(vm->gc->array[v.idx], vm->stack[vm->sp--]);

			SETREG(c.a, min_value(vm->gc, v));
			return;
		}
		break;

	case INSTR_MAX:
		if (vm->sp == 1) {
			SETREG(c.a, max_value(vm->gc, vm->stack[vm->sp--]));
			return;
		} else {
			struct value v;
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->array[v.idx] = new_array();

			while (vm->sp)
				array_push(vm->gc->array[v.idx], vm->stack[vm->sp--]);

			SETREG(c.a, max_value(vm->gc, v));
			return;
		}

		break;

	default:
		DOUT("unimplemented instruction %d (%s)", c.type,
		     instruction_data[c.type].name);
		assert(false);
	}
}

void
execute(struct vm *vm, int64_t ip)
{
	vm->ip = ip;
	vm->returning = false;
	vm->module[vm->fp] = vm->m->id;

	while (vm->code[vm->ip].type != INSTR_END && vm->code[vm->ip].type != INSTR_EEND && vm->fp >= 1 && !vm->returning) {
		/* TODO: remove this */
		assert(vm->impp < 100);
		assert(vm->ip <= vm->m->num_instr);

		if (vm->k->print_code) {
			fprintf(vm->f, "%s:%6zu> %4zu: ", vm->m->name, vm->step, vm->ip);
			print_instruction(vm->f, vm->code[vm->ip]);
			fprintf(vm->f, " | %3zu | %3zu | %3zu | %3zu | %3zu | %3d\n", vm->sp, vm->csp, vm->k->sp, vm->fp, vm->impp, vm->module[vm->fp]);
		}

		execute_instr(vm, vm->code[vm->ip]);
		if (vm->r->pending) break;
		vm->ip++;
		vm->step++;
	}

	if (vm->debug)
		DOUT("Terminating execution of module `%s' with %p: %s",
		     vm->m->name, (void *)vm, !vm->returning ? "finished" : "returned");

	struct oak *k = vm->k;
	k->stack = oak_realloc(k->stack, (k->sp + 1) * sizeof *k->stack);
	if (!vm->escaping) vm->returning = false;
	vm->escaping = false;

	if (vm->code[vm->ip].type == INSTR_END) {
		k->stack[k->sp++] = GETREG(vm->code[vm->ip].a);
	} else if (vm->code[vm->ip].type == INSTR_EEND) {
		push(vm, GETREG(vm->code[vm->ip].a));
		k->stack[k->sp++] = GETREG(vm->code[vm->ip].a);
		ret(vm);
	} else {
		if (vm->sp >= 1) k->stack[k->sp++] = vm->stack[vm->sp];
		else k->stack[k->sp++] = (struct value){ VAL_NIL, {0}, 0 };
	}

	if (vm->r->pending) {
		error_write(vm->r, stderr);
		if (vm->csp) stacktrace(vm);
	}
}

void
vm_panic(struct vm *vm)
{
	/* TODO: clean up memory stuff */
	error_write(vm->r, stderr);
	exit(EXIT_FAILURE);
}
