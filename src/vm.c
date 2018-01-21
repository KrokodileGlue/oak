#include <inttypes.h>
#include <assert.h>

#include "util.h"
#include "vm.h"

void
push_frame(struct vm *vm)
{
	vm->fp++;

	if (vm->fp <= vm->maxfp) {
		vm->module[vm->fp] = false;

		for (int i = 0; i < 256; i++)
			vm->frame[vm->fp][i].type = VAL_UNDEF;
		return;
	}

	vm->frame = oak_realloc(vm->frame, (vm->fp + 1) * sizeof *vm->frame);
	vm->frame[vm->fp] = oak_malloc(256 * sizeof *vm->frame[vm->fp]);

	for (int i = 0; i < 256; i++)
		vm->frame[vm->fp][i].type = VAL_UNDEF;

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
		if (vm->debug)
			DOUT("returning from call as a module");

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

static int
find_undef(struct vm *vm)
{
	int i = 256;
	while (i && GETREG(i - 1).type == VAL_UNDEF) i--;

	/* TODO: error */
	return i;
}

static void
eval(struct vm *vm, int reg, char *s, int scope, struct location loc, int stack_base)
{
	if (vm->debug) fprintf(stderr, "<evaluating '%s'>\n", s);

	vm->module[vm->fp] = true;
	struct module *m = load_module(vm->k,
	                               find_from_scope(vm->m->sym, scope),
	                               strclone(s),
	                               "*eval.k*",
	                               "*eval*",
	                               vm, stack_base);

	if (!m) {
		error_push(vm->r, loc, ERR_FATAL, "eval failed");
		return;
	}

	SETREG(reg, value_translate(vm->gc, m->gc, vm->k->stack[--vm->k->sp]));
	if (!vm->running) vm->ip = vm->callstack[vm->csp--];
	vm->running = true;
	vm->module[vm->fp] = vm->m->child;

	for (int i = stack_base; i < 256; i++)
		if (i != reg) SETR(i, type, VAL_UNDEF);
}

void
execute_instr(struct vm *vm, struct instruction c)
{
	if (vm->debug) {
		fprintf(vm->f, "%s> %4zu: ", vm->m->name, vm->ip);
		print_instruction(vm->f, c);
		fprintf(vm->f, " | %3zu | %3zu | %3zu | %3zu | %s\n", vm->sp, vm->csp, vm->k->sp, vm->fp, vm->module[vm->fp] ? "t" : "f");
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
	case INSTR_INC: SETREG(c.d.a, inc_value(GETREG(c.d.a)));break;
	case INSTR_SUBSCR:
		if (GETREG(c.d.efg.f).type == VAL_ARRAY) {
			if (GETREG(c.d.efg.g).integer >= vm->gc->arrlen[GETREG(c.d.efg.f).idx]
			    || GETREG(c.d.efg.g).integer < 0) {
				struct value v;
				v.type = VAL_NIL;
				SETREG(c.d.efg.e, v);
				break;
			}

			SETREG(c.d.efg.e, copy_value(vm->gc, vm->gc->array[GETREG(c.d.efg.f).idx]
			                             [GETREG(c.d.efg.g).integer]));
		} else if (GETREG(c.d.efg.f).type == VAL_TABLE) {
			SETREG(c.d.efg.e,
			       table_lookup(vm->gc->table[GETREG(c.d.efg.f).idx],
			                    vm->gc->str[GETREG(c.d.efg.g).idx]));
		}
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
		if (GETREG(c.d.efg.e).type != VAL_ARRAY
		    && GETREG(c.d.efg.f).type == VAL_INT) {
			SETR(c.d.efg.e, type, VAL_ARRAY);
			SETR(c.d.efg.e, idx, gc_alloc(vm->gc, VAL_ARRAY));
			vm->gc->arrlen[GETREG(c.d.efg.e).idx] = 0;
			vm->gc->array[GETREG(c.d.efg.e).idx] = NULL;
		}

		if (GETREG(c.d.efg.e).type != VAL_TABLE
		    && GETREG(c.d.efg.f).type == VAL_STR) {
			SETR(c.d.efg.e, type, VAL_TABLE);
			SETR(c.d.efg.e, idx, gc_alloc(vm->gc, VAL_TABLE));
			vm->gc->table[GETREG(c.d.efg.e).idx] = new_table();
		}

		if (GETREG(c.d.efg.f).type == VAL_INT) {
			grow_array(vm->gc, GETREG(c.d.efg.e), GETREG(c.d.efg.f).integer + 1);

			if (vm->gc->arrlen[GETREG(c.d.efg.e).idx] <= GETREG(c.d.efg.f).integer) {
				vm->gc->arrlen[GETREG(c.d.efg.e).idx] = GETREG(c.d.efg.f).integer + 1;
			}

			vm->gc->array[GETREG(c.d.efg.e).idx][GETREG(c.d.efg.f).integer]
				= GETREG(c.d.efg.g);
		}

		if (GETREG(c.d.efg.f).type == VAL_STR) {
			table_add(vm->gc->table[GETREG(c.d.efg.e).idx],
			          vm->gc->str[GETREG(c.d.efg.f).idx],
			          GETREG(c.d.efg.g));
		}
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

	case INSTR_LEQ:
		SETREG(c.d.efg.e, value_leq(vm->gc, GETREG(c.d.efg.f), GETREG(c.d.efg.g)));
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
			vm->gc->str[v.idx] = ktre_filter(re, subject,
			                                 subst, "$");
		} else {
			int **vec = NULL;
			ktre_exec(re, subject, &vec);
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

		SETR(c.d.bc.b, type, VAL_STR);
		int m = vm->match;
		int **vec = ktre_getvec(vm->re);

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

	case INSTR_JOIN: {
		assert(GETREG(c.d.efg.f).type == VAL_STR);
		assert(GETREG(c.d.efg.g).type == VAL_ARRAY);

		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(vm->gc, VAL_STR);
		vm->gc->str[v.idx] = NULL;

		char *a = strclone("");
		size_t arrlen = vm->gc->arrlen[GETREG(c.d.efg.g).idx];

		for (size_t i = 0; i < arrlen - 1; i++) {
			char *b = show_value(vm->gc, vm->gc->array[GETREG(c.d.efg.g).idx][i]);
			a = oak_realloc(a, strlen(a) + strlen(b)
			                + strlen(vm->gc->str[GETREG(c.d.efg.f).idx]) + 1);
			strcat(a, b);
			strcat(a, vm->gc->str[GETREG(c.d.efg.f).idx]);
			free(b);
		}

		char *b = show_value(vm->gc, vm->gc->array[GETREG(c.d.efg.g).idx][arrlen - 1]);
		a = oak_realloc(a, strlen(a) + strlen(b) + 1);
		strcat(a, b);
		free(b);

		vm->gc->str[v.idx] = a;
		SETREG(c.d.efg.e, v);
	} break;

	case INSTR_REV:
		SETREG(c.d.bc.b, rev_value(vm->gc, GETREG(c.d.bc.c)));
		break;

	case INSTR_EVAL:
		if (GETREG(c.d.efg.f).type != VAL_STR) {
			error_push(vm->r, *c.loc, ERR_FATAL, "eval requires string argument");
			return;
		}

		eval(vm, c.d.efg.e, vm->gc->str[GETREG(c.d.efg.f).idx],
		     GETREG(c.d.efg.g).integer, *c.loc, find_undef(vm));
		break;

	case INSTR_NOP:
		break;

	case INSTR_ESCAPE:
		/* Fake a function call return */
		vm->running = false;
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
