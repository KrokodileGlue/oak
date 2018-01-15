#include <inttypes.h>
#include <assert.h>

#include "util.h"
#include "vm.h"

void
push_frame(struct vm *vm)
{
	vm->fp++;
	if (vm->fp <= vm->maxfp) return;

	vm->frame = oak_realloc(vm->frame, (vm->fp + 1) * sizeof *vm->frame);
	vm->frame[vm->fp] = oak_malloc(256 * sizeof *vm->frame[vm->fp]);

	vm->module = oak_realloc(vm->module, (vm->fp + 1) * sizeof *vm->module);
	vm->module[vm->fp] = false;

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

	if (vm->frame)     free(vm->frame);
	if (vm->stack)     free(vm->stack);
	if (vm->callstack) free(vm->callstack);
	if (vm->imp)       free(vm->imp);
	if (vm->subject)   free(vm->subject);
	if (vm->module)    free(vm->module);

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
		m->vm->module[m->vm->fp] = true;
		execute(m->vm, v.integer);
		vm->running = true;

		vm->code = c;
		vm->m = m2;
		vm->ip = ip;
		vm->ct = ct;
		return;

		/* struct module *m = vm->k->modules[v.module]; */
		/* struct vm *vm2 = m->vm; */

		/* size_t sp = vm->sp; */

		/* /\* TODO: only push the arguments the function needs *\/ */
		/* for (size_t i = 1; i <= sp; i++) */
		/* 	push(vm2, value_translate(vm2->gc, vm->gc, vm->stack[i])); */

		/* execute(vm2, v.integer); */
		/* vm->sp -= v.num_args; */
		/* push(vm, value_translate(vm->gc, m->gc, vm->k->stack[--vm->k->sp])); */
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
	if (vm->module[vm->fp]) {
		vm->running = false;
		pop_frame(vm);
		return;
	}

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

#define GETREG(X) ((X) >= 256 ? vm->frame[1][(X) - 256] : vm->frame[vm->fp][X])
#define SETREG(X,Y) ((X) >= 256 ? (vm->frame[1][(X) - 256] = (Y)) : (vm->frame[vm->fp][X] = (Y)))

#define SETR(X,Y,Z) ((X) >= 256 ? (vm->frame[1][(X) - 256].Y = (Z)) : (vm->frame[vm->fp][X].Y = (Z)))

#define CONST(X) (vm->ct->val[X])
#define BIN(X) SETREG(c.d.efg.e, X##_values(vm->gc, GETREG(c.d.efg.f), GETREG(c.d.efg.g)))

void
execute_instr(struct vm *vm, struct instruction c)
{
	if (vm->debug) {
		fprintf(vm->f, "%s> %3zu: ", vm->m->name, vm->ip);
		print_instruction(vm->f, c);
		fprintf(vm->f, " | %3zu | %3zu | %3zu | %3zu\n", vm->sp, vm->csp, vm->k->sp, vm->fp);
	}

	/* Look at all this c.d.bc.c bullshit. Ridiculous. */

	switch (c.type) {
	case INSTR_MOVC: SETREG(c.d.bc.b, CONST(c.d.bc.c));     break;
	case INSTR_LINE: if (!vm->debug) fputc('\n', vm->f);    break;
	case INSTR_MOV:  SETREG(c.d.bc.b, GETREG(c.d.bc.c));    break;
	case INSTR_JMP:  vm->ip = c.d.d - 1;                    break;
	case INSTR_PUSH: push(vm, GETREG(c.d.a));               break;
	case INSTR_POP:  SETREG(c.d.a, pop(vm));                break;
	case INSTR_CALL: call(vm, GETREG(c.d.a));               break;
	case INSTR_RET:  ret(vm);                               break;
	case INSTR_ADD:  BIN(add);                              break;
	case INSTR_SUB:  BIN(sub);                              break;
	case INSTR_MUL:  BIN(mul);                              break;
	case INSTR_POW:  BIN(pow);                              break;
	case INSTR_DIV:  BIN(div);                              break;
	case INSTR_MOD:  BIN(mod);                              break;
	case INSTR_OR:   BIN(or);                               break;
	case INSTR_INC: SETREG(c.d.a, inc_value(GETREG(c.d.a)));break;
	case INSTR_SUBSCR:
		if (GETREG(c.d.efg.g).integer >= vm->gc->arrlen[GETREG(c.d.efg.f).idx]
		    || GETREG(c.d.efg.g).integer < 0) {
			struct value v;
			v.type = VAL_NIL;
			SETREG(c.d.efg.e, v);
			break;
		}

		SETREG(c.d.efg.e, copy_value(vm->gc, vm->gc->array[GETREG(c.d.efg.f).idx]
		                             [GETREG(c.d.efg.g).integer]));
		break;

	case INSTR_FLIP:  SETREG(c.d.bc.b, flip_value(GETREG(c.d.bc.c))); break;
	case INSTR_NEG:   SETREG(c.d.bc.b, neg_value(GETREG(c.d.bc.c))); break;
	case INSTR_COPY:  SETREG(c.d.bc.b, copy_value(vm->gc, GETREG(c.d.bc.c))); break;
	case INSTR_COPYC: SETREG(c.d.bc.b, copy_value(vm->gc, CONST(c.d.bc.c))); break;

	case INSTR_PUSHBACK:
		SETREG(c.d.bc.b, pushback(vm->gc,
		                         GETREG(c.d.bc.b),
		                         GETREG(c.d.bc.c)));
		break;

	case INSTR_ASET:
		/*
		 * TODO: Somewhere we should make sure that the index
		 * is actually an integer.
		 */

		if (GETREG(c.d.efg.e).type != VAL_ARRAY) {
			SETR(c.d.efg.e, type, VAL_ARRAY);
			SETR(c.d.efg.e, idx, gc_alloc(vm->gc, VAL_ARRAY));
			vm->gc->arrlen[GETREG(c.d.efg.e).idx] = 0;
			vm->gc->array[GETREG(c.d.efg.e).idx] = NULL;
		}

		assert(GETREG(c.d.efg.f).type == VAL_INT);
		grow_array(vm->gc, GETREG(c.d.efg.e), GETREG(c.d.efg.f).integer + 1);

		if (vm->gc->arrlen[GETREG(c.d.efg.e).idx] <= GETREG(c.d.efg.f).integer) {
			vm->gc->arrlen[GETREG(c.d.efg.e).idx] = GETREG(c.d.efg.f).integer + 1;
		}

		vm->gc->array[GETREG(c.d.efg.e).idx][GETREG(c.d.efg.f).integer]
			= GETREG(c.d.efg.g);
		break;

	case INSTR_DEREF:
		grow_array(vm->gc, GETREG(c.d.efg.f), GETREG(c.d.efg.g).integer + 1);

		if (vm->gc->array[GETREG(c.d.efg.f).idx][GETREG(c.d.efg.g).integer].type == VAL_ARRAY) {
			SETREG(c.d.efg.e, vm->gc->array[GETREG(c.d.efg.f).idx]
			       [GETREG(c.d.efg.g).integer]);
		} else {
			struct value v;
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(vm->gc, VAL_ARRAY);
			vm->gc->arrlen[v.idx] = 0;
			vm->gc->array[v.idx] = NULL;
			vm->gc->array[GETREG(c.d.efg.f).idx][GETREG(c.d.efg.g).integer] = v;
			SETREG(c.d.efg.e, v);
		}
		break;

	case INSTR_PRINT:
		print_value(vm->f, vm->gc, GETREG(c.d.a));
		if (vm->debug) fputc('\n', stderr);
		break;

	case INSTR_COND:
		if (is_truthy(vm->gc, GETREG(c.d.a)))
			vm->ip++;
		break;

	case INSTR_NCOND:
		if (!is_truthy(vm->gc, GETREG(c.d.a)))
			vm->ip++;
		break;

	case INSTR_CMP:
		SETREG(c.d.efg.e, cmp_values(vm->gc, GETREG(c.d.efg.f), GETREG(c.d.efg.g)));
		break;

	case INSTR_LESS:
		SETREG(c.d.efg.e, value_less(vm->gc, GETREG(c.d.efg.f), GETREG(c.d.efg.g)));
		break;

	case INSTR_MORE:
		SETREG(c.d.efg.e, value_more(vm->gc, GETREG(c.d.efg.f), GETREG(c.d.efg.g)));
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
		vm->impp--;
		break;

	case INSTR_GETIMP:
		SETREG(c.d.a, vm->imp[vm->impp - 1]);
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

		if (vm->subject) free(vm->subject);

		int **vec = NULL;
		struct ktre *re = vm->gc->regex[GETREG(c.d.efg.g).idx];
		char *subject = vm->gc->str[GETREG(c.d.efg.f).idx];
		bool ret = ktre_exec(re, subject, &vec);

		vm->subject = oak_malloc(strlen(subject) + 1);
		strcpy(vm->subject, subject);
		vm->re = re;

		SETR(c.d.efg.e, type, VAL_ARRAY);
		SETR(c.d.efg.e, idx, gc_alloc(vm->gc, VAL_ARRAY));
		vm->gc->array[GETREG(c.d.efg.e).idx] = NULL;
		vm->gc->arrlen[GETREG(c.d.efg.e).idx] = 0;

		if (ret) {
			for (int i = 0; i < re->num_matches; i++) {
				struct value v;
				v.type = VAL_STR;
				v.idx = gc_alloc(vm->gc, VAL_STR);
				vm->gc->str[v.idx] = oak_malloc(vec[i][1] + 1);
				strncpy(vm->gc->str[v.idx], subject + vec[i][0], vec[i][1]);
				vm->gc->str[v.idx][vec[i][1]] = 0;
				pushback(vm->gc, GETREG(c.d.efg.e), v);

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
	} break;

	case INSTR_SUBST: {
		assert(GETREG(c.d.efg.e).type == VAL_STR);
		assert(GETREG(c.d.efg.f).type == VAL_REGEX);
		assert(GETREG(c.d.efg.g).type == VAL_STR);

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = NULL;

		struct ktre *re = vm->gc->regex[GETREG(c.d.efg.f).idx];
		vm->gc->str[v.idx] = ktre_filter(re, vm->gc->str[GETREG(c.d.efg.e).idx],
		                                 vm->gc->str[GETREG(c.d.efg.g).idx], "$");
		SETREG(c.d.efg.e, v);
	} break;

	case INSTR_GROUP: {
		assert(GETREG(c.d.bc.c).type == VAL_INT);

		if (GETREG(c.d.bc.c).integer >= vm->re->num_groups) {
			error_push(vm->r, *c.loc, ERR_FATAL, "group does not exist");
			return;
		}

		SETR(c.d.bc.b, type, VAL_STR);
		int i = vm->re->num_matches - 1;
		int **vec = ktre_getvec(vm->re);

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);

		vm->gc->str[v.idx] = oak_malloc(vec[i][GETREG(c.d.bc.c).integer * 2 + 1] + 1);
		strncpy(vm->gc->str[v.idx], vm->subject + vec[i][GETREG(c.d.bc.c).integer * 2], vec[i][GETREG(c.d.bc.c).integer * 2 + 1]);
		vm->gc->str[v.idx][vec[i][GETREG(c.d.bc.c).integer * 2 + 1]] = 0;

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
		vm->gc->array[GETREG(c.d.efg.e).idx] = NULL;
		vm->gc->arrlen[GETREG(c.d.efg.e).idx] = 0;

		for (int i = 0; i < len; i++) {
			struct value v;
			v.type = VAL_STR;
			v.idx = gc_alloc(vm->gc, VAL_STR);
			vm->gc->str[v.idx] = oak_malloc(strlen(split[i]) + 1);
			strcpy(vm->gc->str[v.idx], split[i]);
			pushback(vm->gc, GETREG(c.d.efg.e), v);

			free(split[i]);
		}

		free(split);
	} break;

	case INSTR_EVAL: {
		if (GETREG(c.d.efg.f).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL, "eval requires string argument");
			return;
		}

		if (vm->debug) fprintf(stderr, "<evaluating '%s'>\n", vm->gc->str[GETREG(c.d.efg.f).idx]);
		struct module *m = load_module(vm->k,
		                               find_from_scope(vm->m->sym, GETREG(c.d.efg.g).integer),
		                               strclone(vm->gc->str[GETREG(c.d.efg.f).idx]),
		                               "*eval.k*",
		                               "*eval*",
		                               vm);

		if (!m) {
			error_push(vm->r, *c.loc, ERR_FATAL, "eval failed");
			return;
		}

		SETREG(c.d.efg.e, value_translate(vm->gc, m->gc, vm->k->stack[--vm->k->sp]));
		if (!vm->running) vm->ip = vm->callstack[vm->csp--];
		vm->running = true;
	} break;

	case INSTR_NOP:
		break;

	case INSTR_ESCAPE:
		/* Fake a function call return */
		vm->running = false;
		vm->callstack = oak_realloc(vm->callstack,
		                            (vm->csp + 2) * sizeof *vm->callstack);
		vm->callstack[++vm->csp] = c.d.a - 1;
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
	vm->running = true;

	while (vm->code[vm->ip].type != INSTR_END && vm->fp >= 1 && vm->running) {
		execute_instr(vm, vm->code[vm->ip]);
		if (vm->r->pending) break;
		vm->ip++;
	}

	if (vm->debug)
		DOUT("Terminating execution of module `%s' with %p: %s",
		     vm->m->name, (void *)vm, vm->fp >= 1 ? "finished" : "returned");

	struct oak *k = vm->k;
	k->stack = oak_realloc(k->stack, (k->sp + 1) * sizeof *k->stack);

	if (vm->code[vm->ip].type == INSTR_END) {
		k->stack[k->sp++] = GETREG(vm->code[vm->ip].d.a);
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
