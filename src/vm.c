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
#define BIN(X)	  \
	do { \
		SETREG(c.d.efg.e, X##_values(vm->gc, GETREG(c.d.efg.f), GETREG(c.d.efg.g))); \
		if (GETREG(c.d.efg.e).type == VAL_ERR) { \
			error_push(vm->r, *c.loc, ERR_FATAL, "%s", GETREG(c.d.efg.e).name); \
		} \
	} while (0)

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

static void
eval(struct vm *vm, int reg, char *s, int scope, struct location loc, int stack_base)
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
		return;
	}

	vm->module[vm->fp] = vm->m->id;
	SETREG(reg, value_translate(vm->gc, m->gc, vm->k->stack[--vm->k->sp]));

	if (vm->returning)
		vm->ip = vm->callstack[vm->csp--];

	vm->returning = false;

	for (int i = stack_base; i < NUM_REG; i++)
		if (i != reg) SETR(i, type, VAL_UNDEF);
}

static inline void
execute_instr(struct vm *vm, struct instruction c)
{
	/* Look at all this c.d.bc.c bullshit. Ridiculous. */

	switch (c.type) {
	case INSTR_MOV:  SETREG(c.d.bc.b, GETREG(c.d.bc.c));    break;
	case INSTR_JMP:  vm->ip = c.d.d - 1;                    break;
	case INSTR_PUSH: push(vm, GETREG(c.d.a));               break;
	case INSTR_POP:  pop(vm, c.d.a);                        break;
	case INSTR_CALL: call(vm, GETREG(c.d.a));               break;
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
	case INSTR_MORE: BIN(more);                             break;
	case INSTR_SLEFT: BIN(sleft);                           break;
	case INSTR_SRIGHT: BIN(sright);                         break;
	case INSTR_INC: SETREG(c.d.a, inc_value(GETREG(c.d.a)));break;
	case INSTR_DEC: SETREG(c.d.a, dec_value(GETREG(c.d.a)));break;
	case INSTR_LINE:
		if (!vm->debug && vm->k->talkative) fputc('\n', vm->f);
		break;

	case INSTR_CHKSTCK:
		/* if (vm->sp) */
		/* 	error_push(vm->r, *c.loc, ERR_FATAL, "invalid number of arguments passed to function (received %d too many)", vm->sp); */
		break;

	case INSTR_FLIP:
		SETREG(c.d.bc.b, flip_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_NEG:
		SETREG(c.d.bc.b, neg_value(GETREG(c.d.bc.c)));
		break;

	case INSTR_COPY:
		SETREG(c.d.bc.b, copy_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_COPYC:
		SETREG(c.d.bc.b, copy_value(vm->gc, CONST(c.d.bc.c)));
		break;

	case INSTR_INT:
		if (GETREG(c.d.bc.c).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "int builtin requires string argument (got %s)",
			           value_data[GETREG(c.d.bc.c).type].body);
			return;
		}

		SETREG(c.d.bc.b, int_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_POPALL: {
		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[v.idx] = new_array();

		for (size_t i = vm->sp; i > 0; i--)
			array_push(vm->gc->array[v.idx], vm->stack[i]);

		SETREG(c.d.a, v);
		vm->sp = 0;
	} break;

	case INSTR_FLOAT:
		if (GETREG(c.d.bc.c).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "float builtin requires string argument (got %s)",
			           value_data[GETREG(c.d.bc.c).type].body);
			return;
		}

		SETREG(c.d.bc.b, float_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_STR: {
		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = show_value(vm->gc, GETREG(c.d.bc.c));
		SETREG(c.d.bc.b, v);
	} break;

	case INSTR_SUBSCR:
		if (GETREG(c.d.efg.f).type == VAL_ARRAY) {
			if (GETREG(c.d.efg.g).type != VAL_INT) {
				error_push(vm->r, *c.loc, ERR_FATAL,
				           "array requires integer subscript (got %s)", value_data[GETREG(c.d.efg.g).type].body);
				return;
			}

			if (GETREG(c.d.efg.g).integer >= vm->gc->array[GETREG(c.d.efg.f).idx]->len
			    || GETREG(c.d.efg.g).integer < 0) {
				struct value v;
				v.type = VAL_NIL;
				SETREG(c.d.efg.e, v);
				break;
			}

			SETREG(c.d.efg.e, vm->gc->array[GETREG(c.d.efg.f).idx]->v
			       [GETREG(c.d.efg.g).integer]);
		} else if (GETREG(c.d.efg.f).type == VAL_TABLE) {
			if (GETREG(c.d.efg.g).type != VAL_STR) {
				error_push(vm->r, *c.loc, ERR_FATAL,
				           "table requires string subscript (got %s)", value_data[GETREG(c.d.efg.g).type].body);
				return;
			}

			SETREG(c.d.efg.e,
			       table_lookup(vm->gc->table[GETREG(c.d.efg.f).idx],
			                    vm->gc->str[GETREG(c.d.efg.g).idx]));
		} else if (GETREG(c.d.efg.f).type == VAL_STR) {
			if (GETREG(c.d.efg.g).type != VAL_INT) {
				error_push(vm->r, *c.loc, ERR_FATAL,
				           "string requires integer subscript (got %s)", value_data[GETREG(c.d.efg.g).type].body);
				return;
			}

			if ((size_t)GETREG(c.d.efg.g).integer <= strlen(vm->gc->str[GETREG(c.d.efg.f).idx])) {
				struct value v;
				v.type = VAL_STR;
				v.idx = gc_alloc(vm->gc, VAL_STR);
				vm->gc->str[v.idx] = strclone(" ");
				vm->gc->str[v.idx][0] = vm->gc->str[GETREG(c.d.efg.f).idx][GETREG(c.d.efg.g).integer];
				SETREG(c.d.efg.e, v);
			} else {
				SETREG(c.d.efg.e, ((struct value){ VAL_NIL, { 0 }, 0}));
			}
		} else {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "invalid subscript on unsubscriptable type %s", value_data[GETREG(c.d.efg.f).type].body);
		}
		break;

	case INSTR_PUSHBACK:
		array_push(vm->gc->array[GETREG(c.d.bc.b).idx],
		           GETREG(c.d.bc.c));
		break;

	case INSTR_ASET:
		if (GETREG(c.d.efg.e).type == VAL_STR
		    && GETREG(c.d.efg.f).type == VAL_INT
		    && ((size_t)GETREG(c.d.efg.f).integer
		        <= strlen(vm->gc->str[GETREG(c.d.efg.e).idx]))) {

			struct value v = GETREG(c.d.efg.e);
			char *a = show_value(vm->gc, GETREG(c.d.efg.g));
			char *b = strclone(vm->gc->str[v.idx]);

			vm->gc->str[v.idx] = oak_realloc(vm->gc->str[v.idx],
			                                 strlen(b)
			                                 + strlen(a) + 1);

			vm->gc->str[v.idx][GETREG(c.d.efg.f).integer] = 0;
			strcat(vm->gc->str[v.idx], a);
			strcat(vm->gc->str[v.idx], b + GETREG(c.d.efg.f).integer + 1);

			free(a);
			free(b);
			return;
		} else if (GETREG(c.d.efg.e).type == VAL_STR
		           && GETREG(c.d.efg.f).type == VAL_INT) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "invalid index into string of length %zu",
			           strlen(vm->gc->str[GETREG(c.d.efg.e).idx]));
			return;
		}

		if (GETREG(c.d.efg.e).type != VAL_ARRAY
		    && GETREG(c.d.efg.f).type == VAL_INT) {
			SETR(c.d.efg.e, type, VAL_ARRAY);
			SETR(c.d.efg.e, idx, gc_alloc(vm->gc, VAL_ARRAY));
			vm->gc->array[GETREG(c.d.efg.e).idx] = new_array();
		}

		if (GETREG(c.d.efg.e).type != VAL_TABLE
		    && GETREG(c.d.efg.f).type == VAL_STR) {
			SETR(c.d.efg.e, type, VAL_TABLE);
			SETR(c.d.efg.e, idx, gc_alloc(vm->gc, VAL_TABLE));
			vm->gc->table[GETREG(c.d.efg.e).idx] = new_table();
		}

		if (GETREG(c.d.efg.e).type == VAL_ARRAY
		    && GETREG(c.d.efg.f).type == VAL_INT) {
			grow_array(vm->gc->array[GETREG(c.d.efg.e).idx], GETREG(c.d.efg.f).integer + 1);

			if (vm->gc->array[GETREG(c.d.efg.e).idx]->len <= GETREG(c.d.efg.f).integer) {
				vm->gc->array[GETREG(c.d.efg.e).idx]->len = GETREG(c.d.efg.f).integer + 1;
			}

			vm->gc->array[GETREG(c.d.efg.e).idx]->v[GETREG(c.d.efg.f).integer]
				= GETREG(c.d.efg.g);
		}

		if (GETREG(c.d.efg.f).type == VAL_STR) {
			table_add(vm->gc->table[GETREG(c.d.efg.e).idx],
			          vm->gc->str[GETREG(c.d.efg.f).idx],
			          GETREG(c.d.efg.g));
		}
		break;

	case INSTR_APUSH: {
		if (GETREG(c.d.bc.b).type != VAL_ARRAY) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "push builtin requires array as its lefthand argument (got %s)",
			           value_data[GETREG(c.d.bc.b).type].body);
			return;
		}

		array_push(vm->gc->array[GETREG(c.d.bc.b).idx], copy_value(vm->gc, GETREG(c.d.bc.c)));
	} break;

	case INSTR_DEREF:
		/* TODO: TABLES! */
		if (GETREG(c.d.efg.f).type != VAL_ARRAY) {
			SETREG(c.d.efg.e, ((struct value){ VAL_NIL, { 0 }, 0}));
			return;
		}

		grow_array(vm->gc->array[GETREG(c.d.efg.f).idx], GETREG(c.d.efg.g).integer + 1);

		if (GETREG(c.d.efg.g).integer < vm->gc->array[GETREG(c.d.efg.f).idx]->len &&
		    vm->gc->array[GETREG(c.d.efg.f).idx]->v[GETREG(c.d.efg.g).integer].type == VAL_ARRAY) {
			SETREG(c.d.efg.e, vm->gc->array[GETREG(c.d.efg.f).idx]->v
			       [GETREG(c.d.efg.g).integer]);
		} else {
			struct value v;
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->array[v.idx] = new_array();

			grow_array(vm->gc->array[GETREG(c.d.efg.f).idx], GETREG(c.d.efg.g).integer + 1);
			vm->gc->array[GETREG(c.d.efg.f).idx]->len = GETREG(c.d.efg.g).integer + 1;
			vm->gc->array[GETREG(c.d.efg.f).idx]->v[GETREG(c.d.efg.g).integer] = v;

			SETREG(c.d.efg.e, v);
		}
		break;

	case INSTR_PRINT:
		if (vm->k->talkative) print_value(vm->f, vm->gc, GETREG(c.d.a));
		if (vm->debug && vm->k->talkative) fputc('\n', stderr);
		break;

	case INSTR_COND:
		if (is_truthy(vm->gc, GETREG(c.d.a)))
			vm->ip++;
		break;

	case INSTR_NCOND:
		if (!is_truthy(vm->gc, GETREG(c.d.a)))
			vm->ip++;
		break;

	case INSTR_TYPE: {
		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);

		vm->gc->str[v.idx] = oak_malloc(strlen(value_data[GETREG(c.d.bc.c).type].body) + 1);
		strcpy(vm->gc->str[v.idx], value_data[GETREG(c.d.bc.c).type].body);

		SETREG(c.d.bc.b, v);
	} break;

	case INSTR_LEN:
		SETREG(c.d.bc.b, value_len(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_KILL:
		assert(GETREG(c.d.a).type == VAL_STR);
		error_push(vm->r, *c.loc, ERR_KILLED, vm->gc->str[GETREG(c.d.a).idx]);
		break;

	case INSTR_PUSHIMP:
		vm->imp = oak_realloc(vm->imp, (vm->impp + 1) * sizeof *vm->imp);
		vm->imp[vm->impp++] = GETREG(c.d.a);
		break;

	case INSTR_POPIMP:
		assert(vm->impp);
		vm->impp--;
		break;

	case INSTR_GETIMP:
		if (vm->impp) SETREG(c.d.a, vm->imp[vm->impp - 1]);
		else error_push(vm->r, *c.loc, ERR_FATAL, "the implicit variable is not in scope");
		break;

	case INSTR_MATCH: {
		if (GETREG(c.d.efg.g).type != VAL_REGEX) {
			error_push(vm->r, *c.loc, ERR_FATAL, "attempt to apply match to non regular expression value");
			return;
		}

		if (GETREG(c.d.efg.f).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL, "attempt to apply regular expression to non-string value");
			return;
		}

		free(vm->subject);
		vm->subject = NULL;

		int **vec = NULL;
		struct ktre *re = vm->gc->regex[GETREG(c.d.efg.g).idx];
		char *subject = vm->gc->str[GETREG(c.d.efg.f).idx];
		bool ret = ktre_exec(re, subject, &vec);

		vm->subject = strclone(subject);
		vm->re = re;

		SETR(c.d.efg.e, type, VAL_ARRAY);
		SETR(c.d.efg.e, idx, gc_alloc(vm->gc, VAL_ARRAY));
		vm->gc->array[GETREG(c.d.efg.e).idx] = new_array();

		if (ret) {
			for (int i = 0; i < re->num_matches; i++) {
				struct value v;
				v.type = VAL_STR;
				v.idx = gc_alloc(vm->gc, VAL_STR);
				vm->gc->str[v.idx] = oak_malloc(vec[i][1] + 1);
				strncpy(vm->gc->str[v.idx], subject + vec[i][0], vec[i][1]);
				vm->gc->str[v.idx][vec[i][1]] = 0;
				array_push(vm->gc->array[GETREG(c.d.efg.e).idx], v);

				for (int j = 1; j < re->num_groups; j++) {
					/* TODO: $1, $2 n stuff */
					/* DOUT("\ngroup %d: `%.*s`", j, vec[i][j * 2 + 1], subject + vec[i][j * 2]); */
				}
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
		assert(GETREG(c.d.efg.e).type == VAL_STR);
		assert(GETREG(c.d.efg.f).type == VAL_REGEX);
		assert(GETREG(c.d.efg.g).type == VAL_STR);

		free(vm->subject);
		vm->subject = NULL;

		struct ktre *re = vm->gc->regex[GETREG(c.d.efg.f).idx];
		char *subject = strclone(vm->gc->str[GETREG(c.d.efg.e).idx]);
		char *subst = vm->gc->str[GETREG(c.d.efg.g).idx];

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = NULL;

		if (GETREG(c.d.efg.f).e == 0) {
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
				SETREG(c.d.efg.e, v);
				return;
			}

			char *a = oak_malloc(128);
			strncpy(a, subject, vec[0][0]);
			a[vec[0][0]] = 0;

			for (int i = 0; i < re->num_matches; i++) {
				char *s = strclone(subst);
				vm->match = i;

				for (int j = 0; j < GETREG(c.d.efg.f).e; j++) {
					free(vm->subject);
					vm->re = re;
					vm->subject = strclone(subject);

					eval(vm, c.d.efg.g, s, c.d.efg.h,
					     *c.loc, find_undef(vm));

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
					s = show_value(vm->gc, GETREG(c.d.efg.g));
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
		SETREG(c.d.efg.e, v);
	} break;

	case INSTR_GROUP: {
		assert(GETREG(c.d.bc.c).type == VAL_INT);

		if (!vm->re) {
			SETR(c.d.bc.b, type, VAL_NIL);
			return;
		}

		if (GETREG(c.d.bc.c).integer >= vm->re->num_groups) {
			error_push(vm->r, *c.loc, ERR_FATAL, "group does not exist");
			return;
		}

		int m = vm->match;
		int **vec = ktre_getvec(vm->re);

		if (m < 0) {
			for (int i = 0; i < vm->re->num_matches; i++)
				free(vec[i]);

			free(vec);
			SETR(c.d.bc.b, type, VAL_NIL);
			return;
		}

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);

		vm->gc->str[v.idx] = oak_malloc(vec[m][GETREG(c.d.bc.c).integer * 2 + 1] + 1);
		strncpy(vm->gc->str[v.idx], vm->subject + vec[m][GETREG(c.d.bc.c).integer * 2], vec[m][GETREG(c.d.bc.c).integer * 2 + 1]);
		vm->gc->str[v.idx][vec[m][GETREG(c.d.bc.c).integer * 2 + 1]] = 0;

		for (int i = 0; i < vm->re->num_matches; i++)
			free(vec[i]);

		free(vec);
		SETREG(c.d.bc.b, v);
	} break;

	case INSTR_SPLIT: {
		int len = 0;
		ktre *re = vm->gc->regex[GETREG(c.d.efg.g).idx];
		char *subject = vm->gc->str[GETREG(c.d.efg.f).idx];
		char **split = ktre_split(re, subject, &len);

		SETR(c.d.efg.e, type, VAL_ARRAY);
		SETR(c.d.efg.e, idx, gc_alloc(vm->gc, VAL_ARRAY));
		vm->gc->array[GETREG(c.d.efg.e).idx] = new_array();

		for (int i = 0; i < len; i++) {
			struct value v;
			v.type = VAL_STR;
			v.idx = gc_alloc(vm->gc, VAL_STR);
			vm->gc->str[v.idx] = oak_malloc(strlen(split[i]) + 1);
			strcpy(vm->gc->str[v.idx], split[i]);
			array_push(vm->gc->array[GETREG(c.d.efg.e).idx], v);

			free(split[i]);
		}

		free(split);
	} break;

	case INSTR_JOIN: {
		assert(GETREG(c.d.efg.f).type == VAL_STR);
		assert(GETREG(c.d.efg.g).type == VAL_ARRAY);

		if (vm->gc->array[GETREG(c.d.efg.g).idx]->len == 0) {
			struct value v;
			v.type = VAL_STR;
			v.idx = gc_alloc(vm->gc, VAL_STR);
			vm->gc->str[v.idx] = strclone("");
			SETREG(c.d.efg.e, v);
			return;
		}

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = NULL;

		char *a = strclone("");
		size_t arrlen = vm->gc->array[GETREG(c.d.efg.g).idx]->len;

		for (size_t i = 0; i < arrlen - 1; i++) {
			char *b = show_value(vm->gc, vm->gc->array[GETREG(c.d.efg.g).idx]->v[i]);
			a = oak_realloc(a, strlen(a) + strlen(b)
			                + strlen(vm->gc->str[GETREG(c.d.efg.f).idx]) + 1);
			strcat(a, b);
			strcat(a, vm->gc->str[GETREG(c.d.efg.f).idx]);
			free(b);
		}

		char *b = show_value(vm->gc, vm->gc->array[GETREG(c.d.efg.g).idx]->v[arrlen - 1]);
		a = oak_realloc(a, strlen(a) + strlen(b) + 1);
		strcat(a, b);
		free(b);

		vm->gc->str[v.idx] = a;
		SETREG(c.d.efg.e, v);
	} break;

	case INSTR_RANGE: {
		assert(GETREG(c.d.efg.f).type == VAL_INT || GETREG(c.d.efg.f).type == VAL_FLOAT);
		assert(GETREG(c.d.efg.g).type == VAL_INT
		       || GETREG(c.d.efg.g).type == VAL_FLOAT
		       || GETREG(c.d.efg.g).type == VAL_ARRAY
		       || GETREG(c.d.efg.g).type == VAL_STR);
		assert(GETREG(c.d.efg.h).type == VAL_INT || GETREG(c.d.efg.h).type == VAL_FLOAT);

		double start = GETREG(c.d.efg.f).type == VAL_INT ? (double)GETREG(c.d.efg.f).integer : GETREG(c.d.efg.f).real;
		double stop = -1;
		double step = GETREG(c.d.efg.h).type == VAL_INT ? (double)GETREG(c.d.efg.h).integer : GETREG(c.d.efg.h).real;

		if (GETREG(c.d.efg.g).type == VAL_ARRAY) {
			stop = (double)vm->gc->array[GETREG(c.d.efg.g).idx]->len - 1;
		} else if (GETREG(c.d.efg.g).type == VAL_STR) {
			stop = strlen(vm->gc->str[GETREG(c.d.efg.g).idx]);
		} else {
			stop = GETREG(c.d.efg.g).type == VAL_INT ? (double)GETREG(c.d.efg.g).integer : GETREG(c.d.efg.g).real;
		}

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[v.idx] = new_array();

		if (fcmp(start, stop)) {
			array_push(vm->gc->array[v.idx], GETREG(c.d.efg.f));
			SETREG(c.d.efg.e, v);
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
				if (GETREG(c.d.efg.f).type == VAL_FLOAT || GETREG(c.d.efg.g).type == VAL_FLOAT)
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
				if (GETREG(c.d.efg.f).type == VAL_FLOAT || GETREG(c.d.efg.g).type == VAL_FLOAT)
					array_push(vm->gc->array[v.idx], (struct value){ VAL_FLOAT, { .real = i }, 0 });
				else
					array_push(vm->gc->array[v.idx], (struct value){ VAL_INT, { .integer = i }, 0 });
			}
		}

		SETREG(c.d.efg.e, v);
	} break;

	case INSTR_REV:
		SETREG(c.d.bc.b, rev_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_SORT:
		SETREG(c.d.bc.b, sort_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_SUM: {
		if (GETREG(c.d.bc.c).type != VAL_ARRAY) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "sum builtin requires array operand (got %s)",
			           value_data[GETREG(c.d.bc.c).type].body);
			return;
		}

		struct value v;
		v.type = VAL_FLOAT;
		v.real = 0;

		for (size_t i = 0; i < vm->gc->array[GETREG(c.d.bc.c).idx]->len; i++) {
			if (vm->gc->array[GETREG(c.d.bc.c).idx]->v[i].type == VAL_INT)
				v.real += vm->gc->array[GETREG(c.d.bc.c).idx]->v[i].integer;

			if (vm->gc->array[GETREG(c.d.bc.c).idx]->v[i].type == VAL_FLOAT)
				v.real += vm->gc->array[GETREG(c.d.bc.c).idx]->v[i].real;
		}

		SETREG(c.d.bc.b, v);
	} break;

	case INSTR_COUNT: {
		if (GETREG(c.d.efg.f).type != VAL_ARRAY) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "count builtin requires array operand (got %s)",
			           value_data[GETREG(c.d.bc.c).type].body);
			return;
		}

		struct value v;
		v.type = VAL_INT;
		v.integer = 0;

		for (size_t i = 0; i < vm->gc->array[GETREG(c.d.bc.c).idx]->len; i++) {
			if (is_truthy(vm->gc,
			              cmp_values(vm->gc,
			                         vm->gc->array[GETREG(c.d.bc.c).idx]->v[i],
			                         GETREG(c.d.efg.g))))
				v.integer++;
		}

		SETREG(c.d.bc.b, v);
	} break;

	case INSTR_ABS: {
		if (GETREG(c.d.bc.c).type != VAL_INT
		    && GETREG(c.d.bc.c).type != VAL_FLOAT) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "abs builtin requires numeric operand (got %s)",
			           value_data[GETREG(c.d.bc.c).type].body);
			return;
		}

		SETREG(c.d.bc.b, abs_value(GETREG(c.d.bc.c)));
	} break;

	case INSTR_EVAL:
		if (GETREG(c.d.efg.f).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "eval requires string argument (got %s)",
			           value_data[GETREG(c.d.efg.f).type].body);
			return;
		}

		eval(vm, c.d.efg.e, vm->gc->str[GETREG(c.d.efg.f).idx],
		     GETREG(c.d.efg.g).integer, *c.loc, find_undef(vm));
		break;

	case INSTR_VALUES:
		if (GETREG(c.d.bc.c).type != VAL_TABLE) {
			error_push(vm->r, *c.loc, ERR_FATAL,
			           "values builtin requires table argument (got %s)",
			           value_data[GETREG(c.d.bc.c).type].body);
			return;
		}

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(vm->gc, VAL_ARRAY);
		vm->gc->array[v.idx] = new_array();

		for (int i = 0; i < TABLE_SIZE; i++) {
			struct bucket b = vm->gc->table[GETREG(c.d.bc.c).idx]->bucket[i];
			for (size_t j = 0; j < b.len; j++) {
				array_push(vm->gc->array[v.idx], b.val[j]);
			}
		}

		SETREG(c.d.bc.b, v);
		break;

	case INSTR_NOP:
		break;

	case INSTR_ESCAPE:
		/* Fake a function call return */
		vm->returning = true;
		vm->escaping = true;
		vm->callstack = oak_realloc(vm->callstack,
		                            (vm->csp + 2) * sizeof *vm->callstack);
		vm->callstack[++vm->csp] = c.d.a - 1;
		break;

	case INSTR_UC:
		assert(GETREG(c.d.bc.c).type == VAL_STR);
		SETREG(c.d.bc.b, uc_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_LC:
		assert(GETREG(c.d.bc.c).type == VAL_STR);
		SETREG(c.d.bc.b, lc_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_UCFIRST:
		assert(GETREG(c.d.bc.c).type == VAL_STR);
		SETREG(c.d.bc.b, ucfirst_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_LCFIRST:
		assert(GETREG(c.d.bc.c).type == VAL_STR);
		SETREG(c.d.bc.b, lcfirst_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_MIN:
		assert(false);
		break;

	case INSTR_MAX:
		if (vm->sp == 1) {
			SETREG(c.d.a, max_value(vm->gc, vm->stack[vm->sp--]));
			return;
		} else {
			struct value v;
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->array[v.idx] = new_array();

			while (vm->sp)
				array_push(vm->gc->array[v.idx], vm->stack[vm->sp--]);

			SETREG(c.d.a, max_value(vm->gc, v));
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
		if (vm->k->print_code) {
			fprintf(vm->f, "%s:%6zu> %4zu: ", vm->m->name, vm->step, vm->ip);
			print_instruction(vm->f, vm->code[vm->ip]);
			fprintf(vm->f, " | %3zu | %3zu | %3zu | %3zu | %3zu | %3d\n", vm->sp, vm->csp, vm->k->sp, vm->fp, vm->impp, vm->module[vm->fp]);
		}

		/* TODO: remove this */
		assert(vm->impp < 100);

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
		k->stack[k->sp++] = GETREG(vm->code[vm->ip].d.a);
	} else if (vm->code[vm->ip].type == INSTR_EEND) {
		push(vm, GETREG(vm->code[vm->ip].d.a));
		k->stack[k->sp++] = GETREG(vm->code[vm->ip].d.a);
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
