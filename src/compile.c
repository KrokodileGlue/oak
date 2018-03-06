#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

#include "constant.h"
#include "constexpr.h"
#include "compile.h"
#include "tree.h"
#include "keyword.h"
#include "util.h"
#include "token.h"
#include "lexer.h"
#include "parse.h"

static struct compiler *
new_compiler()
{
	struct compiler *c = oak_malloc(sizeof *c);

	memset(c, 0, sizeof *c);
	memset(&c->ct, 0, sizeof c->ct);
	c->r = new_reporter();

	return c;
}

static void
free_compiler(struct compiler *c)
{
	free(c->stack_top);
	free(c->stack_base);
	free(c->var);
	free(c->next);
	free(c->last);
	error_clear(c->r);
	free(c);
}

static void
emit(struct compiler *c, struct instruction instr, struct location *loc)
{
	if (!c->instr_alloc) {
		c->instr_alloc = 1024;
		c->code = oak_malloc(c->instr_alloc * sizeof *c->code);
	}

	if ((c->ip + 1) >= c->instr_alloc) {
		c->instr_alloc *= 2;
		c->code = oak_realloc(c->code, c->instr_alloc * sizeof *c->code);
	}

	instr.loc = loc;
	c->code[c->ip++] = instr;
}

static void
emit_(struct compiler *c, enum instruction_type type, struct location *loc)
{
	emit(c, (struct instruction) {type, .a = 0 }, loc);
}

static void
emit_a(struct compiler *c, enum instruction_type type, int a, struct location *loc)
{
	emit(c, (struct instruction) {type, .a = a }, loc);
}

static void
emit_ab(struct compiler *c, enum instruction_type type, int a, int b, struct location *loc)
{
	emit(c, (struct instruction) {type, .a = a, .b = b }, loc);
}

static void
emit_abc(struct compiler *c, enum instruction_type type, int a, int b, int C, struct location *loc)
{
	emit(c, (struct instruction) {type, .a = a, .b = b, .c = C }, loc);
}

static void
emit_abcd(struct compiler *c, enum instruction_type type, int a, int b, int C, int d, struct location *loc)
{
	emit(c, (struct instruction) {type, .a = a, .b = b, .c = C, .d = d }, loc);
}

static void
emit_abcde(struct compiler *c, enum instruction_type type, int a, int b, int C, int d, int e, struct location *loc)
{
	emit(c, (struct instruction) {type, .a = a, .b = b, .c = C, .d = d, .e = e }, loc);
}

static void
push_frame(struct compiler *c)
{
	c->stack_top = oak_realloc(c->stack_top, (c->sp + 2) * sizeof *c->stack_top);
	c->stack_base = oak_realloc(c->stack_base, (c->sp + 2) * sizeof *c->stack_base);
	c->var = oak_realloc(c->var, (c->sp + 2) * sizeof *c->var);

	c->sp++;
	c->var[c->sp] = 0;
	c->stack_top[c->sp] = c->sym->num_variables;
	c->stack_base[c->sp] = c->sym->num_variables;
}

static void
pop_frame(struct compiler *c)
{
	c->sp--;
}

static void
push_last(struct compiler *c, size_t a)
{
	c->last = oak_realloc(c->last, (c->lp + 1) * sizeof *c->last);
	c->last[c->lp++] = a;
}

static void
push_next(struct compiler *c, size_t a)
{
	c->next = oak_realloc(c->next, (c->np + 1) * sizeof *c->next);
	c->next[c->np++] = a;
}

static int
alloc_reg(struct compiler *c)
{
	if (c->stack_top[c->sp] >= 1 << 15) {
		error_push(c->r, c->stmt->tok->loc, ERR_FATAL,
		           "insufficient registers to compile module or function");
		return -1;
	}

	return c->stack_top[c->sp]++;
}

static int
add_constant(struct compiler *c, struct token *tok)
{
	return constant_table_add(c->ct, make_value_from_token(c, tok));
}

static int
nil(struct compiler *c)
{
	int reg = alloc_reg(c);
	emit_ab(c, INSTR_COPYC, reg,
	        constant_table_add(c->ct, NIL),
	        &c->stmt->tok->loc);
	return reg;
}

static int
count_imp(struct symbol *top, struct symbol *base)
{
	int imp = 0;

	while (base && top && base != top->parent) {
		if (base->imp) imp++;
		base = base->parent;
	}

	return imp;
}

static int compile_expression(struct compiler *c, struct expression *e, struct symbol *sym);
static int compile_lvalue(struct compiler *c, struct expression *e, struct symbol *sym);
static int compile_statement(struct compiler *c, struct statement *s);

#define CHECKARGS(X)	  \
	do { \
		if (X) { \
			error_push(c->r, e->tok->loc, ERR_FATAL, \
			           "invalid number of arguments to builtin function `%s'", e->tok->value); \
			return reg; \
		} \
	} while (0)

static int
compile_builtin(struct compiler *c, struct expression *e, struct symbol *sym)
{
	int reg = -1;

	switch (e->bi->name) {
	case BUILTIN_SPLIT: {
		int subject = -1;

		if (e->num == 1) {
			emit_a(c, INSTR_GETIMP, subject = alloc_reg(c), &e->tok->loc);
		} else {
			CHECKARGS(e->num != 2);
			subject = compile_expression(c, e->args[1], sym);
		}

		if (e->args[0]->type == EXPR_REGEX)
			e->args[0]->val->flags = smart_cat(e->args[0]->val->flags, "g");

		int re = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, re, add_constant(c, e->args[0]->val), &e->tok->loc);
		emit_abc(c, INSTR_SPLIT, reg = alloc_reg(c), subject, re, &e->tok->loc);
	} break;

	case BUILTIN_JOIN: {
		CHECKARGS(e->num != 2);
		int array = compile_expression(c, e->args[0], sym);
		int delim = compile_expression(c, e->args[1], sym);
		emit_abc(c, INSTR_JOIN, reg = alloc_reg(c), array, delim, &e->tok->loc);
	} break;

	case BUILTIN_RANGE: {
		CHECKARGS(e->num != 2 && e->num != 3);

		int start = compile_expression(c, e->args[0], sym);
		int stop = compile_expression(c, e->args[1], sym);
		int step = -1;

		if (e->num == 2) {
			step = alloc_reg(c);
			struct value v;
			v.type = VAL_INT;
			v.integer = 1;
			emit_ab(c, INSTR_COPYC, step,
			        constant_table_add(c->ct, v), &e->tok->loc);
		} else {
			step = compile_expression(c, e->args[2], sym);
		}

		emit_abcd(c, INSTR_RANGE, reg = alloc_reg(c), start, stop, step, &e->tok->loc);
	} break;

	case BUILTIN_INSERT: {
		CHECKARGS(e->num != 2 && e->num != 3);

		if (e->num == 3) {
			emit_abc(c, INSTR_INS,
			        reg = compile_lvalue(c, e->args[0], sym),
			        compile_expression(c, e->args[1], sym),
			        compile_expression(c, e->args[2], sym),
			        &e->tok->loc);
		} else {
			emit_abc(c, INSTR_INS,
			        reg = compile_lvalue(c, e->args[0], sym),
			        compile_expression(c, e->args[1], sym),
			        compile_expression(c, NULL, sym),
			        &e->tok->loc);
		}
	} break;

	case BUILTIN_PUSH: {
		CHECKARGS(e->num != 1 && e->num != 2);

		if (e->num == 2) {
			emit_ab(c, INSTR_APUSH,
			        reg = compile_lvalue(c, e->args[0], sym),
			        compile_expression(c, e->args[1], sym),
			        &e->tok->loc);
		} else {
			int imp = alloc_reg(c);
			emit_a(c, INSTR_GETIMP, imp, &e->tok->loc);

			emit_ab(c, INSTR_APUSH,
			        reg = compile_lvalue(c, e->args[0], sym),
			        imp,
			        &e->tok->loc);
		}
	} break;

	case BUILTIN_SAY: {
		int arg = -1;

		if (e->num == 0) {
			arg = alloc_reg(c);
			emit_a(c, INSTR_GETIMP, arg, &e->tok->loc);
			emit_a(c, INSTR_PRINT, arg, &e->tok->loc);
		} else {
			for (size_t i = 0; i < e->num; i++) {
				arg = compile_expression(c, e->args[i], sym);
				emit_a(c, INSTR_PRINT, arg, &e->tok->loc);
			}
		}

		reg = arg;
	} break;

	case BUILTIN_SAYLN: {
		int arg = -1;

		if (e->num == 0) {
			arg = alloc_reg(c);
			emit_a(c, INSTR_GETIMP, arg, &e->tok->loc);
			emit_a(c, INSTR_PRINT, arg, &e->tok->loc);
		} else {
			for (size_t i = 0; i < e->num; i++) {
				arg = compile_expression(c, e->args[i], sym);
				emit_a(c, INSTR_PRINT, arg, &e->tok->loc);
			}
		}

		emit_(c, INSTR_LINE, &e->tok->loc);
		reg = arg;
	} break;

	case BUILTIN_MAP: {
		reg = alloc_reg(c);

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(c->gc, VAL_ARRAY);
		c->gc->array[v.idx] = new_array();
		emit_ab(c, INSTR_COPYC, reg, constant_table_add(c->ct, v), &e->tok->loc);

		int iter = alloc_reg(c);

		v.type = VAL_INT;
		v.integer = -1;

		/* TODO: name these better. */
		int expr = compile_expression(c, e->args[1], sym);
		emit_ab(c, INSTR_COPYC, iter, constant_table_add(c->ct, v), &e->tok->loc);
		size_t start = c->ip;
		emit_a(c, INSTR_INC, iter, &e->tok->loc);
		int len = alloc_reg(c);
		emit_ab(c, INSTR_LEN, len, expr, &e->tok->loc);
		int cond = alloc_reg(c);
		emit_abc(c, INSTR_LESS, cond, iter, len, &e->tok->loc);
		emit_a(c, INSTR_COND, cond, &e->tok->loc);

		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &e->tok->loc);
		int temp = alloc_reg(c);
		emit_abc(c, INSTR_SUBSCR, temp, expr, iter, &e->tok->loc);
		emit_a(c, INSTR_PUSHIMP, temp, &e->tok->loc);

		int thing = compile_expression(c, e->args[0], sym);
		emit_ab(c, INSTR_PUSHBACK, reg, thing, &e->tok->loc);

		emit_(c, INSTR_POPIMP, &e->tok->loc);
		emit_a(c, INSTR_JMP, start, &e->tok->loc);
		c->code[a].a = c->ip;
	} break;

#define UNARY(X,Y)	  \
	case BUILTIN_##X: { \
		int arg = -1; \
		if (e->num == 0) { \
			arg = alloc_reg(c); \
			emit_a(c, INSTR_GETIMP, arg, &e->tok->loc); \
		} else { \
			CHECKARGS(e->num != 1); \
			arg = compile_expression(c, e->args[0], sym); \
		} \
		emit_ab(c, INSTR_##Y, reg = alloc_reg(c), arg, &e->tok->loc); \
	} break

#define CONVERTY(X,Y)	  \
	case BUILTIN_##X: { \
		CHECKARGS(e->num != 1 && e->num != 0); \
		int arg = -1; \
		if (e->num == 0) { \
			arg = alloc_reg(c); \
			emit_a(c, INSTR_GETIMP, arg, &e->tok->loc); \
		} else { \
			arg = compile_expression(c, e->args[0], sym); \
		} \
		reg = alloc_reg(c); \
		emit_ab(c, INSTR_##Y, reg, arg, &e->tok->loc); \
	} break


#define STRETCHY(X,Y)	  \
	case BUILTIN_##X: { \
		reg = alloc_reg(c); \
		if (e->num == 0) { \
			int temp = alloc_reg(c); \
			emit_a(c, INSTR_GETIMP, temp, &e->tok->loc); \
			emit_a(c, INSTR_PUSH, temp, &e->tok->loc); \
			emit_a(c, INSTR_##Y, reg, &e->tok->loc); \
		} else { \
			int arg[e->num]; \
			for (int i = e->num - 1; i >= 0; i--) { \
				arg[i] = alloc_reg(c); \
				emit_ab(c, INSTR_MOV, arg[i], \
				        compile_expression(c, e->args[i], sym), \
				        &e->tok->loc); \
			} \
			for (int i = e->num - 1; i >= 0; i--) \
				emit_a(c, INSTR_PUSH, arg[i], &e->tok->loc); \
			emit_a(c, INSTR_##Y, reg, &e->tok->loc); \
		} \
	} break

#define ARRAYLIKE(X,Y)\
	case BUILTIN_##X: {\
		CHECKARGS(e->num != 1 && e->num != 2);\
		if (e->num == 2) {\
			emit_abc(c, INSTR_##Y, reg = alloc_reg(c),\
			         compile_expression(c, e->args[0], sym),\
			         compile_expression(c, e->args[1], sym),\
			         &e->tok->loc);\
		} else {\
			int imp = alloc_reg(c);\
			emit_a(c, INSTR_GETIMP, imp, &e->tok->loc);\
			emit_abc(c, INSTR_##Y, reg = alloc_reg(c),\
			         imp,\
			         compile_expression(c, e->args[0], sym),\
			         &e->tok->loc);\
		}\
	} break

	ARRAYLIKE(COUNT, COUNT);
	ARRAYLIKE(RJUST, RJUST);

	UNARY(POP, APOP);
	UNARY(SHIFT, SHIFT);
	UNARY(REVERSE, REV);
	UNARY(SORT, SORT);
	UNARY(UC, UC);
	UNARY(LC, LC);
	UNARY(UCFIRST, UCFIRST);
	UNARY(LCFIRST, LCFIRST);
	UNARY(TYPE, TYPE);
	UNARY(LENGTH, LEN);
	UNARY(SUM, SUM);
	UNARY(ABS, ABS);
	UNARY(HEX, HEX);
	UNARY(CHOMP, CHOMP);
	UNARY(TRIM, TRIM);
	UNARY(LASTOF, LASTOF);

	UNARY(KEYS, KEYS);
	UNARY(VALUES, VALUES);

	CONVERTY(INT, INT);
	CONVERTY(FLOAT, FLOAT);
	CONVERTY(STRING, STR);

	STRETCHY(MIN, MIN);
	STRETCHY(MAX, MAX);
	STRETCHY(CHR, CHR);
	STRETCHY(ORD, ORD);

	default:
		DOUT("unimplemented compiler for builtin `%s'",
		     e->bi->body);
		assert(false);
	}

	return reg;
}

static int
write_variable(struct compiler *c, struct expression *e, struct symbol *sym, int rhs)
{
	int reg = -1, r = alloc_reg(c);
	emit_ab(c, INSTR_COPY, r, rhs, &e->tok->loc);

	if (e->type == EXPR_SUBSCRIPT) {
		emit_abc(c, INSTR_ASET,
		         compile_lvalue(c, e->a, sym),
		         compile_expression(c, e->b, sym),
		         reg = r,
		         &e->tok->loc);
	} else if (e->type == EXPR_OPERATOR
	           && e->operator->type == OPTYPE_BINARY
	           && e->operator->name == OP_PERIOD) {
		struct value key;
		key.type = VAL_STR;
		key.idx = gc_alloc(c->gc, VAL_STR);
		c->gc->str[key.idx] = strclone(e->b->val->value);

		int keyreg = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, keyreg,
		        constant_table_add(c->ct, key), &e->tok->loc);

		emit_abc(c, INSTR_ASET,
		         compile_lvalue(c, e->a, sym), keyreg, r,
		         &e->tok->loc);
		reg = r;
	} else {
		int addr = compile_lvalue(c, e, sym);
		emit_ab(c, INSTR_MOV, addr, r, &e->tok->loc);
		reg = r;
	}

	return reg;
}

static int
compile_operator(struct compiler *c, struct expression *e, struct symbol *sym)
{
	int reg = -1;

#define BIN(X,Y)	  \
	case OP_##X: \
		emit_abc(c, INSTR_##Y, reg = alloc_reg(c), \
		         compile_expression(c, e->a, sym), \
		         compile_expression(c, e->b, sym), \
		         &e->tok->loc); \
		break

	switch (e->operator->type) {
	case OPTYPE_BINARY:
		switch (e->operator->name) {
			BIN(ADD, ADD);
			BIN(MUL, MUL);
			BIN(DIV, DIV);
			BIN(SUB, SUB);
			BIN(MOD, MOD);
			BIN(LEFT, SLEFT);
			BIN(RIGHT, SRIGHT);

			BIN(BAND, BAND);
			BIN(XOR, XOR);
			BIN(BOR, BOR);
			BIN(CMP, CMP);
			BIN(LESS, LESS);
			BIN(MORE, MORE);
			BIN(LEQ, LEQ);
			BIN(GEQ, GEQ);

		case OP_NOTEQ: {
			int temp = alloc_reg(c);
			emit_abc(c, INSTR_CMP, temp,
			         compile_expression(c, e->a, sym),
			         compile_expression(c, e->b, sym), &e->tok->loc);
			emit_ab(c, INSTR_FLIP, reg = alloc_reg(c), temp, &e->tok->loc);
		} break;

		case OP_CC:
			emit_ab(c, INSTR_MOV,
			        reg = alloc_reg(c),
			        compile_expression(c, e->b,
			                           resolve(sym, e->a->val->value)->children[0]),
			        &e->tok->loc);
			break;

		case OP_EQ:
			write_variable(c, e->a, sym, reg = compile_expression(c, e->b, sym));
			break;

		case OP_AND: {
			reg = alloc_reg(c);
			struct value v;
			v.type = VAL_BOOL;
			v.boolean = false;
			emit_ab(c, INSTR_COPYC, reg, constant_table_add(c->ct, v), &e->tok->loc);
			emit_a(c, INSTR_COND, compile_expression(c, e->a, sym), &e->a->tok->loc);
			int a = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->a->tok->loc);
			emit_a(c, INSTR_COND, compile_expression(c, e->b, sym), &e->b->tok->loc);
			int b = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->b->tok->loc);
			v.boolean = true;
			emit_ab(c, INSTR_COPYC, reg, constant_table_add(c->ct, v), &e->tok->loc);
			c->code[a].a = c->ip;
			c->code[b].a = c->ip;
		} break;

		case OP_OR: {
			reg = alloc_reg(c);
			struct value v;
			v.type = VAL_BOOL;
			v.boolean = false;
			emit_ab(c, INSTR_COPYC, reg, constant_table_add(c->ct, v), &e->tok->loc);

			int a = compile_expression(c, e->a, sym);

			emit_a(c, INSTR_COND, a, &e->a->tok->loc);
			int _a = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->a->tok->loc);
			v.boolean = true;
			emit_ab(c, INSTR_COPYC, reg, constant_table_add(c->ct, v), &e->tok->loc);
			int _c = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->a->tok->loc);

			c->code[_a].a = c->ip;
			int b = compile_expression(c, e->b, sym);

			emit_a(c, INSTR_COND, b, &e->b->tok->loc);
			int _b = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->b->tok->loc);

			v.boolean = true;
			emit_ab(c, INSTR_COPYC, reg, constant_table_add(c->ct, v), &e->tok->loc);

			c->code[_b].a = c->ip;
			c->code[_c].a = c->ip;
		} break;

		case OP_ARROW: {
			int start = compile_expression(c, e->a, sym);
			int stop = compile_expression(c, e->b, sym);

			int step = alloc_reg(c);
			emit_ab(c, INSTR_COPYC, step,
			        constant_table_add(c->ct, INT(1)), &e->tok->loc);

			emit_abcd(c, INSTR_RANGE, reg = alloc_reg(c), start, stop, step, &e->tok->loc);
		} break;

		case OP_IFF: {
			reg = alloc_reg(c);
			emit_a(c, INSTR_COND, compile_expression(c, e->b, sym), &e->tok->loc);
			size_t a = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->tok->loc);
			emit_ab(c, INSTR_MOV, reg, compile_expression(c, e->a, sym), &e->tok->loc);
			size_t b = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->tok->loc);
			c->code[a].a = c->ip;
			emit_ab(c, INSTR_MOV, reg, nil(c), &e->tok->loc);
			c->code[b].a = c->ip;
		} break;

#define OPEQ(X,Y)	  \
	case OP_##X: { \
		reg = alloc_reg(c); \
		emit_abc(c, INSTR_##Y, reg, compile_expression(c, e->a, sym), compile_expression(c, e->b, sym), &e->tok->loc); \
		write_variable(c, e->a, sym, reg); \
	} break

		OPEQ(ADDEQ, ADD);
		OPEQ(SUBEQ, SUB);
		OPEQ(MULEQ, MUL);
		OPEQ(DIVEQ, DIV);

		case OP_DOTEQ: {
			int t = alloc_reg(c);
			emit_abc(c, INSTR_ADD, t, compile_expression(c, e->a, sym), reg = compile_expression(c, e->b, sym), &e->tok->loc);
			write_variable(c, e->a, sym, t);
		} break;

		case OP_COMMA:
			compile_expression(c, e->a, sym);
			reg = compile_expression(c, e->b, sym);
			break;

		case OP_POW:
			reg = alloc_reg(c);
			emit_abc(c, INSTR_POW, reg, compile_expression(c, e->a, sym), compile_expression(c, e->b, sym), &e->tok->loc);
			break;

		case OP_PERIOD: {
			reg = alloc_reg(c);

			if (e->b->type == EXPR_VALUE && e->b->tok->type == TOK_IDENTIFIER) {
				struct value key;
				key.type = VAL_STR;
				key.idx = gc_alloc(c->gc, VAL_STR);
				c->gc->str[key.idx] = strclone(e->b->tok->value);

				int keyreg = alloc_reg(c);
				emit_ab(c, INSTR_MOVC, keyreg,
				        constant_table_add(c->ct, key), &e->tok->loc);

				emit_abc(c, INSTR_SUBSCR,
				         reg,
				         compile_expression(c, e->a, sym),
				         keyreg,
				         &e->tok->loc);
			} else {
				error_push(c->r, e->tok->loc, ERR_FATAL,
				           "binary . requires identifier righthand argument");
				return -1;
			}
		} break;

		case OP_MODMOD: {
			reg = alloc_reg(c);
			int temp = alloc_reg(c);
			emit_abc(c, INSTR_MOD, temp,
			         compile_expression(c, e->a, sym),
			         compile_expression(c, e->b, sym),
			         &e->tok->loc);

			struct value v;
			v.type = VAL_INT;
			v.integer = 0;

			int zero = alloc_reg(c);
			emit_ab(c, INSTR_COPYC, zero, constant_table_add(c->ct, v), &e->tok->loc);

			emit_abc(c, INSTR_CMP, reg,
			         temp,
			         zero,
			         &e->tok->loc);
		} break;

		case OP_SQUIGGLEEQ: {
			if (e->b->type != EXPR_REGEX) {
				error_push(c->r, e->tok->loc, ERR_FATAL,
				           "operator requires regex righthand argument");
				return -1;
			}

			if (e->b->val->substitution) {
				int temp = alloc_reg(c);
				emit_ab(c, INSTR_COPY, temp, compile_expression(c, e->a, sym), &e->tok->loc);
				reg = temp;

				/* Load stuff. */
				int str = alloc_reg(c), re = alloc_reg(c);
				emit_ab(c, INSTR_COPYC, re, add_constant(c, e->b->val), &e->tok->loc);
				struct value v;
				v.type = VAL_STR;
				v.idx = gc_alloc(c->gc, VAL_STR);
				c->gc->str[v.idx] = strclone(e->b->val->substitution);
				emit_ab(c, INSTR_COPYC, str, constant_table_add(c->ct, v), &e->tok->loc);

				emit_abcd(c, INSTR_SUBST, temp,
				          re, str, sym->scope, &e->tok->loc);
				write_variable(c, e->a, sym, temp);
			} else {
				int re = alloc_reg(c);
				reg = alloc_reg(c);
				emit_ab(c, INSTR_COPYC, re, add_constant(c, e->b->val), &e->tok->loc);
				emit_abc(c, INSTR_MATCH, reg, compile_expression(c, e->a, sym), re, &e->b->tok->loc);
			}
		} break;

		default:
			DOUT("unimplemented compiler for binary operator `%s'",
			     e->operator->body);
			assert(false);
		}
		break;

#define PRE(X,Y)	  \
		case OP_##X: { \
			emit_a(c, INSTR_##Y, reg = compile_expression(c, e->a, sym), &e->tok->loc); \
			write_variable(c, e->a, sym, reg); \
		} break

	case OPTYPE_PREFIX:
		switch (e->operator->name) {
		PRE(ADDADD, INC);
		PRE(SUBSUB, DEC);

		case OP_SUB:
			reg = alloc_reg(c);
			emit_ab(c, INSTR_NEG, reg, compile_expression(c, e->a, sym), &e->tok->loc);
			break;

		case OP_ADD:
			return compile_expression(c, e->a, sym);

		case OP_EXCLAMATION:
			reg = alloc_reg(c);
			emit_ab(c, INSTR_FLIP, reg, compile_expression(c, e->a, sym), &e->tok->loc);
			break;

		default:
			DOUT("unimplemented compiler for prefix operator `%s'",
			     e->operator->body);
			assert(false);
		}
		break;

#define POST(X,Y)	  \
		case OP_##X: { \
			reg = compile_expression(c, e->a, sym); \
			int t = alloc_reg(c); \
			emit_ab(c, INSTR_COPY, t, reg, &e->tok->loc); \
			emit_a(c, INSTR_##Y, t, &e->tok->loc); \
			write_variable(c, e->a, sym, t); \
		} break

	case OPTYPE_POSTFIX:
		switch (e->operator->name) {
			POST(ADDADD, INC);
			POST(SUBSUB, DEC);

		default:
			DOUT("unimplemented compiler for postfix operator `%s'",
			     e->operator->body);
			assert(false);
		}
		break;

	case OPTYPE_TERNARY:
		switch (e->operator->name) {
		case OP_QUESTION: {
			reg = alloc_reg(c);
			emit_a(c, INSTR_COND, compile_expression(c, e->a, sym), &e->tok->loc);
			size_t a = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->tok->loc);
			emit_ab(c, INSTR_MOV, reg, compile_expression(c, e->b, sym), &e->tok->loc);
			size_t b = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->tok->loc);
			c->code[a].a = c->ip;
			emit_ab(c, INSTR_MOV, reg, compile_expression(c, e->c, sym), &e->tok->loc);
			c->code[b].a = c->ip;
		} break;

		default:
			DOUT("unimplemented compiler for ternary operator `%s'",
			     e->operator->body);
			assert(false);
		}
		break;

	default:
		DOUT("unimplemented operator compiler for type `%d'",
		     e->operator->type);
		assert(false);
	}

	return reg;
}

static int
compile_lvalue(struct compiler *c, struct expression *e, struct symbol *sym)
{
	switch (e->type) {
	case EXPR_VALUE:
		switch (e->val->type) {
		case TOK_IDENTIFIER: {
			struct symbol *var = resolve(sym, e->val->value);

			if (var->type != SYM_VAR && var->type != SYM_ARGUMENT) {
				error_push(c->r, e->tok->loc, ERR_FATAL, "invalid use of identifier in lvalue");
				return -1;
			}

			return var->address;
		} break;

		default:
			error_push(c->r, e->tok->loc, ERR_FATAL, "expected an lvalue");
			return -1;
		}
		break;

	case EXPR_SUBSCRIPT: {
		int reg = alloc_reg(c);
		int l = compile_lvalue(c, e->a, sym);
		emit_abc(c, INSTR_DEREF, reg,
		         l,
		         compile_expression(c, e->b, sym),
		         &e->tok->loc);
		return reg;
	}

	case EXPR_OPERATOR: {
		if (e->operator->type != OPTYPE_BINARY && e->operator->name != OP_PERIOD) {
			error_push(c->r, e->tok->loc, ERR_FATAL,
			           "invalid operation in lvalue");
			return -1;
		}

		if (e->b->type == EXPR_VALUE && e->b->val->type != TOK_IDENTIFIER) {
			error_push(c->r, e->tok->loc, ERR_FATAL,
			           "binary . requires identifier righthand argument");
			return -1;
		}

		struct value key;
		key.type = VAL_STR;
		key.idx = gc_alloc(c->gc, VAL_STR);
		c->gc->str[key.idx] = strclone(e->b->tok->value);

		int keyreg = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, keyreg,
		        constant_table_add(c->ct, key), &e->tok->loc);

		int reg = alloc_reg(c);
		int l = compile_lvalue(c, e->a, sym);
		emit_abc(c, INSTR_DEREF, reg, l, keyreg, &e->tok->loc);

		return reg;
	}

	default:
		error_push(c->r, e->tok->loc, ERR_FATAL, "expected an lvalue");
		return -1;
	}

	assert(false);
}

static int
compile_expression(struct compiler *c, struct expression *e, struct symbol *sym)
{
	assert(sym);
	int reg = -1;

	if (!e) {
		reg = alloc_reg(c);
		emit_a(c, INSTR_GETIMP, reg, &c->stmt->tok->loc);
		return reg;
	}

	if (is_constant_expr(c, sym, e)) {
		emit_ab(c, INSTR_COPYC, reg = alloc_reg(c), constant_table_add(c->ct, compile_constant_expr(c, sym, e)), &e->tok->loc);
		return reg;
	}

	switch (e->type) {
	case EXPR_BUILTIN:  reg = compile_builtin(c, e, sym);  break;
	case EXPR_OPERATOR: reg = compile_operator(c, e, sym); break;

	case EXPR_VALUE:
		switch (e->val->type) {
		case TOK_IDENTIFIER:
			reg = alloc_reg(c);

			if (!strcmp(e->val->value, "_")) {
				emit_a(c, INSTR_GETIMP, reg, &e->tok->loc);
				return reg;
			}

			if (!strcmp(e->val->value, "nil"))
				return nil(c);

			struct symbol *var = resolve(sym, e->val->value);
			assert(var);

			if (var->type == SYM_ENUM) {
				emit_ab(c, INSTR_COPYC, reg = alloc_reg(c), constant_table_add(c->ct, INT(var->_enum)), &e->tok->loc);
			} else if (var->type == SYM_FN) {
				struct value v;
				v.type = VAL_FN;
				v.num_args = var->num_arguments;
				v.integer = var->address;
				v.module = var->module->id;
				v.name = var->name;
				emit_ab(c, INSTR_COPYC, reg = alloc_reg(c), constant_table_add(c->ct, v), &e->tok->loc);
			} else {
				emit_ab(c, INSTR_MOV, reg, var->address, &e->tok->loc);
			}
			break;

		case TOK_STRING: {
			if (e->val->is_interpolatable) {
				int temp = alloc_reg(c);
				emit_ab(c, INSTR_COPYC, temp, add_constant(c, e->val), &e->tok->loc);
				emit_abc(c, INSTR_INTERP, reg = alloc_reg(c), temp, sym->scope, &e->tok->loc);
			} else {
				emit_ab(c, INSTR_COPYC, reg = alloc_reg(c), add_constant(c, e->val), &e->tok->loc);
			}
		} break;

		default:
			emit_ab(c, INSTR_COPYC, reg = alloc_reg(c), add_constant(c, e->val), &e->tok->loc);
			break;
		}
		break;

	case EXPR_TABLE: {
		reg = alloc_reg(c);

		struct value v;
		v.type = VAL_TABLE;
		v.idx = gc_alloc(c->gc, VAL_TABLE);
		c->gc->table[v.idx] = new_table();

		emit_ab(c, INSTR_COPYC, reg, constant_table_add(c->ct, v), &e->tok->loc);

		for (size_t i = 0; i < e->num; i++) {
			struct value key;
			key.type = VAL_STR;
			key.idx = gc_alloc(c->gc, VAL_STR);

			char *a = oak_malloc(strlen(e->keys[i]->value) + 1);

			if (e->keys[i]->type == TOK_IDENTIFIER) strcpy(a, e->keys[i]->value);
			else strcpy(a, e->keys[i]->string);

			c->gc->str[key.idx] = a;

			int keyreg = alloc_reg(c);
			emit_ab(c, INSTR_COPYC, keyreg,
			        constant_table_add(c->ct, key), &e->tok->loc);

			emit_abc(c, INSTR_ASET,
			         reg,
			         keyreg,
			         compile_expression(c, e->args[i], sym),
			         &e->tok->loc);
		}
		break;
	} break;

	/* TODO: clean this up */
	case EXPR_MATCH: {
		reg = nil(c);
		int last = -1;
		int end[e->num];

		emit_a(c, INSTR_PUSHIMP,
		       compile_expression(c, e->a, sym), &e->tok->loc);

		for (size_t i = 0; i < e->num; i++) {
			if (last >= 0)
				c->code[last].a = c->ip;

			int cond = -1;
			if (e->match[i]->type == EXPR_REGEX) {
				int temp = alloc_reg(c);
				int re = alloc_reg(c);
				emit_ab(c, INSTR_COPYC, re, add_constant(c, e->match[i]->val), &e->tok->loc);
				cond = alloc_reg(c);

				emit_a(c, INSTR_GETIMP, temp, &e->tok->loc);
				emit_abc(c, INSTR_MATCH, cond, temp,
				         re,
				         &e->match[i]->tok->loc);
				emit_a(c, INSTR_COND, cond, &e->tok->loc);
			} else {
				cond = alloc_reg(c);
				int imp = alloc_reg(c);
				emit_a(c, INSTR_GETIMP, imp, &e->tok->loc);
				emit_abc(c, INSTR_CMP, cond,
				         imp,
				         compile_expression(c, e->match[i], sym),
				         &e->match[i]->tok->loc);
				emit_a(c, INSTR_COND, cond, &e->tok->loc);
			}

			last = c->ip;
			emit_a(c, INSTR_JMP, cond, &e->tok->loc);

			if (e->args[i]) {
				emit_ab(c, INSTR_MOV, reg, compile_expression(c, e->args[i], sym), &e->tok->loc);
			} else {
				if (e->bodies[i]->block.stmts[e->bodies[i]->block.num - 1]->type == STMT_EXPR) {
					for (size_t j = 0; j < e->bodies[i]->block.num - 1; j++)
						compile_statement(c, e->bodies[i]->block.stmts[j]);
				} else {
					for (size_t j = 0; j < e->bodies[i]->block.num; j++)
						compile_statement(c, e->bodies[i]->block.stmts[j]);
				}

				if (e->bodies[i]->block.stmts[e->bodies[i]->block.num - 1]->type == STMT_EXPR) {
					struct statement *it = e->bodies[i]->block.stmts[e->bodies[i]->block.num - 1];
					emit_ab(c, INSTR_MOV, reg,
					        compile_expression(c, it->expr, find_from_scope(sym, it->scope)),
					        &e->tok->loc);
				} else {
					reg = nil(c);
				}
			}

			end[i] = c->ip;
			emit_a(c, INSTR_JMP, -1, &e->tok->loc);
		}

		for (size_t i = 0; i < e->num; i++)
			c->code[end[i]].a = c->ip;

		if (last >= 0)
			c->code[last].a = c->ip;

		emit_(c, INSTR_POPIMP, &e->tok->loc);
		break;
	} break;

	case EXPR_FN_CALL: {
		int arg[e->num];

		for (int i = e->num - 1; i >= 0; i--) {
			arg[i] = alloc_reg(c);
			emit_ab(c, INSTR_MOV, arg[i], compile_expression(c, e->args[i], sym), &e->tok->loc);
		}

		for (int i = e->num - 1; i >= 0; i--)
			emit_a(c, INSTR_PUSH, arg[i], &e->tok->loc);

		if (e->a->type == EXPR_OPERATOR
		    && e->a->operator->type == OPTYPE_BINARY
		    && e->a->operator->name == OP_PERIOD) {
			int table = compile_expression(c, e->a->a, sym);
			emit_a(c, INSTR_PUSH, table, &e->tok->loc);

			/*
			 * TODO: Make sure everyone using key or
			 * keyreg are properly checking that the thing
			 * is an identifier!
			 */
			struct value key;
			key.type = VAL_STR;
			key.idx = gc_alloc(c->gc, VAL_STR);
			c->gc->str[key.idx] = strclone(e->a->b->tok->value);

			int keyreg = alloc_reg(c);
			emit_ab(c, INSTR_MOVC, keyreg,
			        constant_table_add(c->ct, key), &e->tok->loc);

			int fn = alloc_reg(c);
			emit_abc(c, INSTR_SUBSCR, fn, table, keyreg, &e->tok->loc);
			emit_a(c, INSTR_CALL, fn, &e->tok->loc);
		} else emit_a(c, INSTR_CALL, compile_expression(c, e->a, sym), &e->tok->loc);

		emit_a(c, INSTR_POP, reg = alloc_reg(c), &e->tok->loc);
	} break;

	case EXPR_FN_DEF: {
		struct value v;
		v.type = VAL_FN;
		v.num_args = e->s->fn_def.num;
		v.module = sym->module->id;
		v.integer = compile_statement(c, e->s);
		v.name = NULL;
		emit_ab(c, INSTR_COPYC, reg = alloc_reg(c), constant_table_add(c->ct, v), &e->tok->loc);
	} break;

	case EXPR_LIST: {
		reg = alloc_reg(c);
		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(c->gc, VAL_ARRAY);
		c->gc->array[v.idx] = new_array();

		emit_ab(c, INSTR_COPYC, reg, constant_table_add(c->ct, v), &e->tok->loc);

		for (size_t i = 0; i < e->num; i++)
			emit_ab(c, INSTR_PUSHBACK, reg, compile_expression(c, e->args[i], sym), &e->tok->loc);
	} break;

	case EXPR_SUBSCRIPT: {
		int array = compile_expression(c, e->a, sym);
		reg = alloc_reg(c);
		emit_abc(c, INSTR_SUBSCR, reg, array,
		         compile_expression(c, e->b, sym), &e->tok->loc);
	} break;

	case EXPR_SLICE: {
		int array = compile_expression(c, e->a, sym);
		reg = alloc_reg(c);

		int B, C, D;

		if (!e->b) emit_ab(c, INSTR_COPYC, B = alloc_reg(c), constant_table_add(c->ct, NIL), &e->tok->loc);
		else B = compile_expression(c, e->b, sym);

		if (!e->c) emit_ab(c, INSTR_COPYC, C = alloc_reg(c), constant_table_add(c->ct, NIL), &e->tok->loc);
		else C = compile_expression(c, e->c, sym);

		if (!e->d) emit_ab(c, INSTR_COPYC, D = alloc_reg(c), constant_table_add(c->ct, INT(1)), &e->tok->loc);
		else D = compile_expression(c, e->d, sym);

		emit_abcde(c, INSTR_SLICE, reg, array, B, C, D, &e->tok->loc);
	} break;

	case EXPR_REGEX: {
		int re = alloc_reg(c);
		int subj = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, re, add_constant(c, e->val), &e->tok->loc);
		emit_a(c, INSTR_GETIMP, subj, &e->tok->loc);
		emit_abc(c, INSTR_MATCH, reg = alloc_reg(c), subj, re, &e->tok->loc);
	} break;

	case EXPR_GROUP: {
		reg = alloc_reg(c);
		int temp = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, temp, add_constant(c, e->tok), &e->tok->loc);
		emit_ab(c, INSTR_GROUP, reg, temp, &e->tok->loc);
	} break;

	case EXPR_EVAL: {
		if (c->debug)
			printf("compiling eval with %d\n", sym->scope);
		struct value v;
		v.type = VAL_INT;
		v.integer = sym->scope;
		int a = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, a, constant_table_add(c->ct, v), &e->tok->loc);

		emit_abc(c, INSTR_EVAL,
		        reg = alloc_reg(c),
		        compile_expression(c, e->a, sym),
		        a,
		        &e->tok->loc);
	} break;

	case EXPR_LIST_COMPREHENSION: {
		int array = -1;
		int index = alloc_reg(c);

		if (e->b) {
			array = compile_expression(c, e->b, sym);
		} else {
			array = compile_expression(c, e->s->expr, sym);
		}

		reg = alloc_reg(c);

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(c->gc, VAL_ARRAY);
		c->gc->array[v.idx] = new_array();

		emit_ab(c, INSTR_COPYC, reg,
		        constant_table_add(c->ct, v),
		        &e->tok->loc);

		emit_ab(c, INSTR_COPYC, index,
		        constant_table_add(c->ct, INT(-1)),
		        &e->tok->loc);

		int cond = alloc_reg(c);
		size_t start = c->ip;

		emit_a(c, INSTR_INC, index, &e->tok->loc);
		int len = alloc_reg(c);
		emit_ab(c, INSTR_LEN, len, array, &e->tok->loc);

		emit_abc(c, INSTR_LESS, cond, index, len, &e->tok->loc);
		emit_a(c, INSTR_COND, cond, &e->tok->loc);
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &e->tok->loc);

		if (e->s->type == STMT_EXPR && e->b) {
			assert(e->s->expr->type == EXPR_VALUE);
			struct symbol *var = resolve(sym, e->s->expr->val->value);
			emit_abc(c, INSTR_SUBSCR, var->address, array, index, &e->tok->loc);
		} else if (e->s->type == STMT_VAR_DECL && e->b) {
			struct statement *s = e->s;

			if (s->var_decl.num != 1) {
				error_push(c->r, e->tok->loc, ERR_FATAL,
				           "variable declarations in list comprehensions may declare only one variable");
				return -1;
			}

			sym = find_from_scope(sym, s->scope);
			struct symbol *var = resolve(sym, s->var_decl.names[0]->value);
			var->address = c->var[c->sp]++;
			emit_abc(c, INSTR_SUBSCR, var->address, array, index, &e->tok->loc);
		} else if (e->b) {
			assert(false);
		}

		if (!e->b) {
			int imp = alloc_reg(c);
			emit_abc(c, INSTR_SUBSCR, imp, array, index, &e->tok->loc);
			emit_a(c, INSTR_PUSHIMP, imp, &e->tok->loc);
		}

		emit_ab(c, INSTR_PUSHBACK, reg,
		        compile_expression(c, e->a, sym), &e->tok->loc);

		if (!e->b) emit_(c, INSTR_POPIMP, &e->tok->loc);

		emit_a(c, INSTR_JMP, start, &e->tok->loc);
		c->code[a].a = c->ip;
	} break;

	case EXPR_VARARGS:
		emit_a(c, INSTR_POPALL, reg = alloc_reg(c), &e->tok->loc);
		break;

	default:
		DOUT("unimplemented compiler for expression of type `%d'",
		     e->type);
		assert(false);
	}

	return reg;
}

static void
set_stack_top(struct compiler *c)
{
	if (c->in_expr) return;
	c->stack_top[c->sp] = c->stack_base[c->sp];
}

static int
compile_expr(struct compiler *c, struct expression *e, struct symbol *sym)
{
	set_stack_top(c);
	c->in_expr = true;
	int t = compile_expression(c, e, sym);
	c->in_expr = false;

	return t;
}

#define LOOP_FINISH	  \
	do { \
		for (int i = np; i < c->np; i++) \
			c->code[c->next[i]].a = sym->next; \
		c->np = np; \
		for (int i = lp; i < c->lp; i++) \
			c->code[c->last[i]].a = c->ip; \
		c->lp = lp; \
	} while (0)

/* jesus christ */
static int
compile_for_loop(struct compiler *c, struct statement *s, struct symbol *sym,
                 int lp, int np)
{
	int start = c->ip;
	struct statement *old_loop = c->loop;
	c->loop = s;

	if (s->for_loop.a && s->for_loop.b && !s->for_loop.c
	           && s->for_loop.b->type == EXPR_OPERATOR
	           && s->for_loop.b->operator->type == OPTYPE_BINARY
	           && s->for_loop.b->operator->name == OP_SQUIGGLEEQ) {
		/* for var j; i =~ /\w(\d+)/g: pl '> $1 - $j' */

		int reg;
		if (s->for_loop.a->type == STMT_VAR_DECL) {
			if (s->for_loop.a->var_decl.num != 1) {
				error_push(c->r, s->tok->loc, ERR_FATAL,
				           "variable declaration for-loop initalizers must declare only one variable");
			}

			compile_statement(c, s->for_loop.a);
			reg = resolve(sym, s->for_loop.a->var_decl.names[0]->value)->address;
		} else {
			reg = compile_lvalue(c, s->for_loop.a->expr, sym);
		}

		struct expression *e = s->for_loop.b;
		int expr = c->var[c->sp]++;
		int operand = c->var[c->sp]++;
		int re = c->var[c->sp]++;

		e->b->val->flags = smart_cat(e->b->val->flags, "c");
		emit_ab(c, INSTR_MOV, operand, compile_expr(c, e->a, sym), &s->tok->loc);
		set_stack_top(c);
		emit_ab(c, INSTR_COPYC, re, add_constant(c, e->b->val), &e->tok->loc);
		emit_a(c, INSTR_RESETR, re, &s->tok->loc);

		start = c->ip;
		emit_abc(c, INSTR_MATCH, expr, operand, re, &e->b->tok->loc);

		emit_a(c, INSTR_COND, expr, &s->tok->loc);
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);

		int zero = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, zero, constant_table_add(c->ct, INT(0)), &s->tok->loc);
		int temp = alloc_reg(c);
		emit_abc(c, INSTR_SUBSCR, temp, expr, zero, &s->tok->loc);

		emit_ab(c, INSTR_MOV, reg, temp, &s->tok->loc);
		compile_statement(c, s->for_loop.body);

		emit_a(c, INSTR_JMP, start, &s->tok->loc);
		int b = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);

		set_next(sym, start);
		c->code[a].a = c->ip;
		c->code[b].a = c->ip;
	} else if (s->for_loop.a && s->for_loop.b && !s->for_loop.c
	           && s->for_loop.b->type == EXPR_REGEX) {
		/* for var j; /\w(\d+)/g: pl '> $1 - $j' */

		int reg;
		if (s->for_loop.a->type == STMT_VAR_DECL) {
			if (s->for_loop.a->var_decl.num != 1) {
				error_push(c->r, s->tok->loc, ERR_FATAL,
				           "variable declaration for-loop initalizers must declare only one variable");
			}

			compile_statement(c, s->for_loop.a);
			reg = resolve(sym, s->for_loop.a->var_decl.names[0]->value)->address;
		} else {
			reg = compile_lvalue(c, s->for_loop.a->expr, sym);
		}

		struct expression *e = s->for_loop.b;
		int expr = c->var[c->sp]++;
		int operand = c->var[c->sp]++;
		int re = c->var[c->sp]++;

		e->val->flags = smart_cat(e->val->flags, "c");
		set_stack_top(c);
		emit_ab(c, INSTR_COPYC, re, add_constant(c, e->val), &e->tok->loc);
		emit_a(c, INSTR_RESETR, re, &s->tok->loc);

		emit_a(c, INSTR_GETIMP, operand, &e->tok->loc);
		start = c->ip;
		emit_abc(c, INSTR_MATCH, expr, operand, re, &e->tok->loc);

		emit_a(c, INSTR_COND, expr, &s->tok->loc);
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);

		int zero = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, zero, constant_table_add(c->ct, INT(0)), &s->tok->loc);
		int temp = alloc_reg(c);
		emit_abc(c, INSTR_SUBSCR, temp, expr, zero, &s->tok->loc);

		emit_ab(c, INSTR_MOV, reg, temp, &s->tok->loc);
		compile_statement(c, s->for_loop.body);

		emit_a(c, INSTR_JMP, start, &s->tok->loc);
		int b = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);

		set_next(sym, start);
		c->code[a].a = c->ip;
		c->code[b].a = c->ip;
	} else if (s->for_loop.a && s->for_loop.b && s->for_loop.c) {
		if (s->for_loop.a->type == STMT_EXPR) {
			compile_expr(c, s->for_loop.a->expr, sym);
		} else if (s->for_loop.a->type == STMT_VAR_DECL) {
			compile_statement(c, s->for_loop.a);
		}

		size_t a = c->ip;
		emit_a(c, INSTR_COND, compile_expr(c, s->for_loop.b, sym), &s->tok->loc);
		size_t b = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		compile_statement(c, s->for_loop.body);
		start = c->ip;
		compile_expression(c, s->for_loop.c, sym);

		emit_a(c, INSTR_JMP, a, &s->tok->loc);
		c->code[b].a = c->ip;

		set_next(sym, start);
	} else if (s->for_loop.a && s->for_loop.b && !s->for_loop.c) {
		int reg = -1;
		if (s->for_loop.a->type == STMT_VAR_DECL) {
			if (s->for_loop.a->var_decl.num != 1) {
				error_push(c->r, s->tok->loc, ERR_FATAL,
				           "variable declaration for-loop initalizers must declare only one variable");
			}

			compile_statement(c, s->for_loop.a);
			reg = resolve(sym, s->for_loop.a->var_decl.names[0]->value)->address;
		} else {
			reg = compile_lvalue(c, s->for_loop.a->expr, sym);
		}

		int iter = c->var[c->sp]++;

		struct value v;
		v.type = VAL_INT;
		v.integer = -1;

		int expr = c->var[c->sp]++;
		emit_ab(c, INSTR_MOV, expr, compile_expr(c, s->for_loop.b, sym), &s->tok->loc);
		emit_ab(c, INSTR_COPYC, iter, constant_table_add(c->ct, v), &s->tok->loc);
		start = c->ip;
		emit_a(c, INSTR_INC, iter, &s->tok->loc);
		int len = alloc_reg(c);
		emit_ab(c, INSTR_LEN, len, expr, &s->tok->loc);
		int cond = alloc_reg(c);
		emit_abc(c, INSTR_LESS, cond, iter, len, &s->tok->loc);
		emit_a(c, INSTR_COND, cond, &s->tok->loc);
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		emit_abc(c, INSTR_SUBSCR, reg, expr, iter, &s->tok->loc);

		compile_statement(c, s->for_loop.body);
		emit_a(c, INSTR_JMP, start, &s->tok->loc);

		set_next(sym, start);
		c->code[a].a = c->ip;
	} else if (s->for_loop.a && !s->for_loop.b && !s->for_loop.c
	           && s->for_loop.a->type == STMT_EXPR
	           && s->for_loop.a->expr->type == EXPR_OPERATOR
	           && s->for_loop.a->expr->operator->type == OPTYPE_BINARY
	           && s->for_loop.a->expr->operator->name == OP_SQUIGGLEEQ) {
		/* TODO: care if e->a or e->b actually exist */

		struct expression *e = s->for_loop.a->expr;
		int expr = c->var[c->sp]++;
		int operand = c->var[c->sp]++;
		int re = c->var[c->sp]++;

		e->b->val->flags = smart_cat(e->b->val->flags, "c");
		emit_ab(c, INSTR_MOV, operand, compile_expr(c, e->a, sym), &s->tok->loc);
		set_stack_top(c);
		emit_ab(c, INSTR_COPYC, re, add_constant(c, e->b->val), &e->tok->loc);
		emit_a(c, INSTR_RESETR, re, &s->tok->loc);

		start = c->ip;
		emit_abc(c, INSTR_MATCH, expr, operand, re, &e->b->tok->loc);

		emit_a(c, INSTR_COND, expr, &s->tok->loc);
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);

		int zero = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, zero, constant_table_add(c->ct, INT(0)), &s->tok->loc);
		int temp = alloc_reg(c);
		emit_abc(c, INSTR_SUBSCR, temp, expr, zero, &s->tok->loc);

		emit_a(c, INSTR_PUSHIMP, temp, &s->tok->loc);
		compile_statement(c, s->for_loop.body);
		emit_(c, INSTR_POPIMP, &s->tok->loc);

		emit_a(c, INSTR_JMP, start, &s->tok->loc);
		int b = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);

		set_next(sym, start);
		c->code[a].a = c->ip;
		c->code[b].a = c->ip;
	} else if (s->for_loop.a && !s->for_loop.b && !s->for_loop.c
	           && s->for_loop.a->type == STMT_EXPR
	           && s->for_loop.a->expr->type == EXPR_REGEX) {
		int expr = c->var[c->sp]++;
		int re = c->var[c->sp]++;
		s->for_loop.a->expr->val->flags = smart_cat(s->for_loop.a->expr->val->flags, "c");

		set_stack_top(c);
		emit_ab(c, INSTR_COPYC, re, add_constant(c, s->for_loop.a->expr->val), &s->tok->loc);
		emit_a(c, INSTR_RESETR, re, &s->tok->loc);

		start = c->ip;
		emit_abc(c, INSTR_MATCH, expr,
		         compile_expression(c, NULL, sym),
		         re, &s->tok->loc);

		emit_a(c, INSTR_COND, expr, &s->tok->loc);
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);

		int zero = alloc_reg(c);
		emit_ab(c, INSTR_COPYC, zero, constant_table_add(c->ct, INT(0)), &s->tok->loc);
		int temp = alloc_reg(c);
		emit_abc(c, INSTR_SUBSCR, temp, expr, zero, &s->tok->loc);

		emit_a(c, INSTR_PUSHIMP, temp, &s->tok->loc);
		compile_statement(c, s->for_loop.body);
		emit_(c, INSTR_POPIMP, &s->tok->loc);

		emit_a(c, INSTR_JMP, start, &s->tok->loc);
		int b = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);

		set_next(sym, start);
		c->code[a].a = c->ip;
		c->code[b].a = c->ip;
	} else if (s->for_loop.a && !s->for_loop.b && !s->for_loop.c) {
		if (s->for_loop.a->type != STMT_EXPR) {
			error_push(c->r, s->tok->loc, ERR_FATAL,
			           "argument to `for' must be an expression");
			return start;
		}

		int iter = c->var[c->sp]++;
		int expr = c->var[c->sp]++;

		emit_ab(c, INSTR_MOV, expr, compile_expr(c, s->for_loop.a->expr, sym), &s->tok->loc);
		emit_ab(c, INSTR_COPYC, iter, constant_table_add(c->ct, INT(-1)), &s->tok->loc);

		start = c->ip;
		emit_a(c, INSTR_INC, iter, &s->tok->loc);
		int len = alloc_reg(c);
		emit_ab(c, INSTR_LEN, len, expr, &s->tok->loc);
		int cond = alloc_reg(c);
		emit_abc(c, INSTR_LESS, cond, iter, len, &s->tok->loc);
		emit_a(c, INSTR_COND, cond, &s->tok->loc);
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		int temp = alloc_reg(c);
		emit_abc(c, INSTR_SUBSCR, temp, expr, iter, &s->tok->loc);

		emit_a(c, INSTR_PUSHIMP, temp, &s->tok->loc);
		compile_statement(c, s->for_loop.body);
		emit_(c, INSTR_POPIMP, &s->tok->loc);

		emit_a(c, INSTR_JMP, start, &s->tok->loc);
		int e = c->ip;
		emit_a(c, INSTR_JMP, start, &s->tok->loc);
		emit_(c, INSTR_POPIMP, &s->tok->loc);

		set_next(sym, start);
		c->code[a].a = c->ip;
		c->code[e].a = c->ip;
	}

	c->loop = old_loop;
	set_last(sym, c->ip);
	LOOP_FINISH;
	return start;
}

static int
compile_statement(struct compiler *c, struct statement *s)
{
	if (!s) return -1;
	int ret = c->ip;

	struct symbol *sym = find_from_scope(c->m->sym, s->scope);
	if (c->debug) {
		printf("%d> compiling %d (%s) in `%s' (%d)\n", c->sym->scope, s->type,
		        statement_data[s->type].body, sym->name, sym->scope);
	}

	if (s->type == STMT_VAR_DECL && s->condition) {
		error_push(c->r, s->condition->tok->prev->loc, ERR_FATAL,
		           "thou shalt not fuck with variable declarations");
		return -1;
	}

	size_t start = -1;
	if (s->condition) {
		emit_a(c, INSTR_COND,
		       compile_expr(c, s->condition, sym), &s->tok->loc);
		start = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);
	}

	int lp = c->lp;
	int np = c->np;

	switch (s->type) {
	case STMT_PRINTLN:
		for (size_t i = 0; i < s->print.num; i++) {
			emit_a(c, INSTR_PRINT,
			       compile_expr(c, s->print.args[i], sym),
			       &s->tok->loc);
		}
		emit_(c, INSTR_LINE, &s->tok->loc);
		break;

	case STMT_PRINT:
		for (size_t i = 0; i < s->print.num; i++) {
			emit_a(c, INSTR_PRINT,
			       compile_expr(c, s->print.args[i], sym),
			       &s->tok->loc);
		}
		break;

	case STMT_VAR_DECL:
		for (size_t i = 0; i < s->var_decl.num; i++) {
			struct symbol *var_sym = resolve(sym, s->var_decl.names[i]->value);
			var_sym->address = c->var[c->sp]++;
			if (var_sym->global) var_sym->address += NUM_REG;
			int reg = -1;

			reg = s->var_decl.init
				? compile_expr(c, s->var_decl.init[i], sym)
				: nil(c);

			emit_ab(c, INSTR_COPY, var_sym->address, reg, &s->tok->loc);
		}
		break;

	case STMT_FN_DEF: {
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		push_frame(c);
		sym->address = c->ip;
		ret = c->ip;

		for (size_t i = 0; i < s->fn_def.num; i++) {
			struct symbol *arg_sym = resolve(sym, s->fn_def.args[i]->value);
			arg_sym->address = c->var[c->sp]++;

			if (s->fn_def.init[i]) {
				emit_ab(c, INSTR_MOV,
				        arg_sym->address,
				        compile_expression(c, s->fn_def.init[i], sym),
				        &s->tok->loc);
			} else {
				emit_ab(c, INSTR_MOV,
				        arg_sym->address,
				        nil(c),
				        &s->tok->loc);
			}

			emit_a(c, INSTR_POP, arg_sym->address, &s->tok->loc);
		}

		emit_(c, INSTR_CHKSTCK, &s->tok->loc);
		compile_statement(c, s->fn_def.body);
		pop_frame(c);
		emit_a(c, INSTR_PUSH, nil(c), &s->tok->loc);
		emit_(c, INSTR_RET, &s->tok->loc);
		c->code[a].a = c->ip;
	} break;

	case STMT_RET:
		/*
		 * TODO: Mark symbols as in-function or out so we can
		 * tell statically whether it's okay to return from
		 * inside of an eval.
		 */

		if (c->sp == 0 && c->eval) {
			emit_a(c, INSTR_EEND, compile_expr(c, s->ret.expr, sym), &c->stmt->tok->loc);
			return start;
		}

		if (c->sp == 0 && !c->eval) {
			error_push(c->r, s->tok->loc, ERR_FATAL,
			           "'return' keyword must occur inside of a function body");
			return -1;
		}

		for (int i = 0; i < count_imp(find_from_scope(sym, c->loop->scope), sym); i++)
			emit_(c, INSTR_POPIMP, &s->tok->loc);

		emit_a(c, INSTR_PUSH, compile_expr(c, s->ret.expr, sym), &s->tok->loc);
		emit_(c, INSTR_RET, &s->tok->loc);
		break;

	case STMT_BLOCK:
		for (size_t i = 0; i < s->block.num; i++)
			compile_statement(c, s->block.stmts[i]);
		break;

	case STMT_EXPR:
		compile_expr(c, s->expr, sym);
		break;

	case STMT_IF_STMT: {
		emit_a(c, INSTR_COND, compile_expr(c, s->if_stmt.cond, sym), &s->tok->loc);
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		compile_statement(c, s->if_stmt.then);
		size_t b = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		c->code[a].a = c->ip;
		compile_statement(c, s->if_stmt.otherwise);
		c->code[b].a = c->ip;
	} break;

	case STMT_WHILE: {
		struct statement *old_loop = c->loop;
		c->loop = s;

		size_t a = c->ip;
		emit_a(c, INSTR_COND, compile_expr(c, s->while_loop.cond, sym), &s->tok->loc);
		size_t b = c->ip;
		emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		compile_statement(c, s->while_loop.body);
		emit_a(c, INSTR_JMP, a, &s->tok->loc);
		c->code[b].a = c->ip;

		set_next(sym, a);
		set_last(sym, c->ip);
		LOOP_FINISH;

		c->loop = old_loop;
	} break;

	case STMT_DO: {
		struct statement *old_loop = c->loop;
		c->loop = s;

		size_t a = c->ip;
		compile_statement(c, s->do_while_loop.body);
		for (int i = np; i < c->np; i++)
			c->code[c->next[i]].a = c->ip;
		c->np = np;
		emit_a(c, INSTR_NCOND, compile_expr(c, s->do_while_loop.cond, sym), &s->tok->loc);
		emit_a(c, INSTR_JMP, a, &s->tok->loc);
		for (int i = lp; i < c->lp; i++)
			c->code[i].a = c->ip;
		c->lp = lp;

		set_next(sym, a);
		set_last(sym, c->ip);
		c->loop = old_loop;
	} break;

	case STMT_FOR_LOOP: start = compile_for_loop(c, s, sym, lp, np); break;

	case STMT_LAST:
		for (int i = 0; i < count_imp(find_from_scope(c->sym, c->loop->scope), sym); i++)
			emit_(c, INSTR_POPIMP, &s->tok->loc);

		if (c->eval) {
			if (sym->last < 0) {
				error_push(c->r, s->tok->loc, ERR_FATAL,
				           "'next' keyword must occur inside of a loop body");
				return -1;
			}

			emit_a(c, INSTR_ESCAPE, sym->last, &s->tok->loc);
		} else {
			push_last(c, c->ip);
			emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		}
		break;

	case STMT_NEXT: {
		for (int i = 0; i < count_imp(find_from_scope(c->sym, c->loop->scope), sym); i++)
			emit_(c, INSTR_POPIMP, &s->tok->loc);

		if (c->eval) {
			if (sym->next < 0) {
				error_push(c->r, s->tok->loc, ERR_FATAL,
				           "'next' keyword must occur inside of a loop body");
				return -1;
			}

			emit_a(c, INSTR_ESCAPE, sym->next, &s->tok->loc);
		} else {
			push_next(c, c->ip);
			emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		}
	} break;

	case STMT_DIE:
		emit_a(c, INSTR_KILL, compile_expr(c, s->expr, sym),
		       &s->tok->loc);
		break;

	case STMT_LABEL: {
		struct symbol *l = resolve(sym, s->label);
		l->address = c->ip;
		for (int i = 0; i < l->labelp; i++)
			c->code[l->label[i]].a = l->address;
	} break;

	case STMT_GOTO: {
		struct symbol *l = resolve(sym, s->label);

		if (!l) {
			error_push(c->r, s->tok->loc, ERR_FATAL, "undeclared identifier");
			return -1;
		}

		if (sym->fp != 0 && l->global) {
			error_push(c->r, s->tok->loc, ERR_FATAL, "goto may not jump out of a function definition");
			return -1;
		}

		int imp = 0;
		struct symbol *sy = sym;
		while (sy != l->parent) {
			if (sy->imp) imp++;
			sy = sy->parent;
		}

		for (int i = 0; i < imp; i++)
			emit_(c, INSTR_POPIMP, &s->tok->loc);

		if (l->address != (size_t)-1) {
			emit_a(c, INSTR_JMP, l->address, &s->tok->loc);
		} else {
			l->label = oak_realloc(l->label, (l->labelp + 1) * sizeof *l->label);
			l->label[l->labelp++] = c->ip;
			emit_a(c, INSTR_JMP, -1, &s->tok->loc);
		}
	} break;

	case STMT_ENUM: {
		int cur = 0;

		for (size_t i = 0; i < s->_enum.num; i++) {
			if (s->_enum.init[i] && is_constant_expr(c, sym, s->_enum.init[i])) {
				struct value v = compile_constant_expr(c, sym, s->_enum.init[i]);

				if (v.type != VAL_INT) {
					error_push(c->r, s->_enum.names[i]->loc, ERR_FATAL, "enum initalizer requires integer expression");
					return -1;
				}

				cur = v.integer;
			} else if (s->_enum.init[i]) {
				error_push(c->r, s->_enum.names[i]->loc, ERR_FATAL, "enum initalizer requires constant expression");
				return -1;
			}

			resolve(sym, s->_enum.names[i]->value)->_enum = cur;
			cur++;
		}
	} break;

	case STMT_IMPORT:
	case STMT_NULL:
		break;

	default:
		DOUT("unimplemented compiler for statement of type %d (%s)",
		     s->type, statement_data[s->type].body);
		assert(false);
	}

	if (s->condition)
		c->code[start].a = c->ip;

	return ret;
}

bool
compile(struct module *m, struct constant_table *ct, struct symbol *sym,
        bool debug, bool eval, int stack_base)
{
	struct compiler *c = new_compiler();

	c->m = m;
	c->eval = eval;
	c->sym = sym;
	c->num_nodes = m->num_nodes;
	c->stmt = m->tree[0];
	c->gc = m->gc;
	c->debug = debug;
	c->sp = -1;
	c->loop = m->tree[0];

	if (ct) c->ct = ct;
	else c->ct = new_constant_table();

	push_frame(c);
	if (stack_base >= 0) {
		c->var[c->sp] = stack_base;
		struct symbol *s = find_from_scope(sym, m->tree[0]->scope);
		c->stack_base[c->sp] = stack_base + s->num_variables;
	}

	for (size_t i = 0; i < c->num_nodes; i++) {
		if (i == c->num_nodes - 1 && m->tree[i]->type == STMT_EXPR) {
			int reg = compile_expr(c, m->tree[i]->expr,
			                       find_from_scope(c->m->sym,
			                                       m->tree[i]->scope));
			emit_a(c, INSTR_END, reg,
			       &c->stmt->tok->loc);
			break;
		}

		c->stmt = m->tree[i];
		compile_statement(c, m->tree[i]);
	}

	emit_a(c, INSTR_END, nil(c), &c->stmt->tok->loc);

	m->code = c->code;
	m->num_instr = c->ip;
	m->ct = c->ct;
	m->stage = MODULE_STAGE_COMPILED;

	if (c->np) {
		for (int i = 0; i < c->np; i++) {
			error_push(c->r, *c->code[c->next[i]].loc, ERR_FATAL,
			           "'next' keyword must occur inside of a loop body");
		}
	}

	if (c->lp) {
		for (int i = 0; i < c->lp; i++) {
			error_push(c->r, *c->code[c->last[i]].loc, ERR_FATAL,
			           "'last' keyword must occur inside of a loop body");
		}
	}

	if (c->r->fatal) {
		error_write(c->r, stdout);
		free_compiler(c);
		return false;
	} else if (c->r->pending) {
		error_write(c->r, stdout);
	}

	free_compiler(c);
	return true;
}
