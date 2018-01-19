#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

#include "constant.h"
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
	emit(c, (struct instruction){type, .d = { 0 }}, loc);
}

static void
emit_a(struct compiler *c, enum instruction_type type, int a, struct location *loc)
{
	emit(c, (struct instruction){type, .d = { a }}, loc);
}

static void
emit_d(struct compiler *c, enum instruction_type type, int a, struct location *loc)
{
	emit(c, (struct instruction){type, .d = { .d = a }}, loc);
}

static void
emit_bc(struct compiler *c, enum instruction_type type, int B, int C, struct location *loc)
{
	emit(c, (struct instruction){type, .d = { .bc = {B, C} }}, loc);
}

static void
emit_efg(struct compiler *c, enum instruction_type type, int E, int F, int G, struct location *loc)
{
	if (type == INSTR_CMP)
		assert(E >= 0);
	emit(c, (struct instruction){type, .d = { .efg = {E, F, G} }}, loc);
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
	if (c->stack_top[c->sp] >= 256) {
		error_push(c->r, c->stmt->tok->loc, ERR_FATAL,
		           "insufficient registers to compile module or function");
		return -1;
	}

	return c->stack_top[c->sp]++;
}

static struct value
make_value_from_token(struct compiler *c, struct token *tok)
{
	struct value v;

	switch (tok->type) {
	case TOK_STRING: {
		v.type = VAL_STR;
		v.idx = gc_alloc(c->gc, VAL_STR);
		c->gc->str[v.idx] = oak_malloc(strlen(tok->string) + 1);
		strcpy(c->gc->str[v.idx], tok->string);
	} break;

	case TOK_INTEGER:
		v.type = VAL_INT;
		v.integer = tok->integer;
		break;

	case TOK_FLOAT:
		v.type = VAL_FLOAT;
		v.real = tok->floating;
		break;

	case TOK_BOOL:
		v.type = VAL_BOOL;
		v.boolean = tok->boolean;
		break;

	case TOK_REGEX: {
		int opt = 0;
		int e = 0;

		for (size_t i = 0; i < strlen(tok->flags); i++) {
			switch (tok->flags[i]) {
			case 'i': opt |= KTRE_INSENSITIVE; break;
			case 'g': opt |= KTRE_GLOBAL; break;
			case 'e': e++; break;
			default: error_push(c->r, tok->loc, ERR_FATAL, "unrecognized flag `%c'", tok->flags[i]);
			}
		}

		v.type = VAL_REGEX;
		v.idx = gc_alloc(c->gc, VAL_REGEX);
		v.e = e;
		c->gc->regex[v.idx] = ktre_compile(tok->regex, opt | KTRE_UNANCHORED);
	} break;

	case TOK_GROUP:
		v.type = VAL_INT;
		v.integer = tok->integer;
		break;

	default:
		DOUT("unimplemented token-to-value converter for token of type %d", tok->type);
		assert(false);
	}

	return v;
}

static int
add_constant(struct compiler *c, struct token *tok)
{
	return constant_table_add(c->ct, make_value_from_token(c, tok));
}

static int
nil(struct compiler *c)
{
	struct value v;
	v.type = VAL_NIL;
	int n = constant_table_add(c->ct, v);
	int reg = alloc_reg(c);
	emit_bc(c, INSTR_MOVC, reg, n, &c->stmt->tok->loc);
	return reg;
}

static int compile_expression(struct compiler *c, struct expression *e, struct symbol *sym, bool copy);
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
		CHECKARGS(e->num != 2);

		if (e->args[0]->type == EXPR_REGEX)
			e->args[0]->val->flags = smart_cat(e->args[0]->val->flags, "g");

		int regex = compile_expression(c, e->args[0], sym, false);
		int subject = compile_expression(c, e->args[1], sym, false);
		emit_efg(c, INSTR_SPLIT, reg = alloc_reg(c), subject, regex, &e->tok->loc);
	} break;

	case BUILTIN_JOIN: {
		CHECKARGS(e->num != 2);

		int array = compile_expression(c, e->args[0], sym, false);
		int delim = compile_expression(c, e->args[1], sym, false);
		emit_efg(c, INSTR_JOIN, reg = alloc_reg(c), array, delim, &e->tok->loc);
	} break;

#define UNARY(X,Y)	  \
	case BUILTIN_##X: { \
		int arg = -1; \
		if (e->num == 0) { \
			arg = alloc_reg(c); \
			emit_a(c, INSTR_GETIMP, arg, &e->tok->loc); \
		} else { \
			CHECKARGS(e->num != 1); \
			arg = compile_expression(c, e->args[0], sym, false); \
		} \
		emit_bc(c, INSTR_##Y, reg = alloc_reg(c), arg, &e->tok->loc);\
	} break

	UNARY(REVERSE, REV);
	UNARY(UC, UC);
	UNARY(LC, LC);
	UNARY(UCFIRST, UCFIRST);
	UNARY(LCFIRST, LCFIRST);

	default:
		DOUT("unimplemented compiler for builtin `%s'",
		     e->bi->body);
		assert(false);
	}

	return reg;
}

static int
compile_operator(struct compiler *c, struct expression *e, struct symbol *sym)
{
	int reg = -1;

#define o(X)	  \
	case OP_##X: \
		emit_efg(c, INSTR_##X, reg = alloc_reg(c), \
		         compile_expression(c, e->a, sym, false), \
		         compile_expression(c, e->b, sym, false), \
		         &e->tok->loc); \
		break

	switch (e->operator->type) {
	case OPTYPE_BINARY:
		switch (e->operator->name) {
			o(ADD);
			o(MUL);
			o(DIV);
			o(SUB);
			o(MOD);

		case OP_EQ:
			if (e->a->type == EXPR_SUBSCRIPT) {
				reg = compile_lvalue(c, e->a->a, sym);
				emit_efg(c, INSTR_ASET,
				         reg,
				         compile_expression(c, e->a->b, sym, false),
				         compile_expression(c, e->b, sym, true),
				         &e->tok->loc);
			} else {
				int addr = compile_lvalue(c, e->a, sym);
				emit_bc(c, INSTR_MOV, addr,
				        compile_expression(c, e->b, sym, true),
				        &e->tok->loc);
				reg = addr;
			}
			break;

		case OP_ADDEQ:
			if (e->a->type == EXPR_SUBSCRIPT) {
				assert(false);
			} else {
				int addr = compile_lvalue(c, e->a, sym);
				int n = alloc_reg(c);
				emit_bc(c, INSTR_MOV, n, addr, &e->tok->loc);
				emit_efg(c, INSTR_ADD, addr, n, compile_expression(c, e->b, sym, false), &e->tok->loc);
				reg = addr;
			}
			break;

		case OP_EQEQ:
			reg = alloc_reg(c);
			emit_efg(c, INSTR_CMP, reg, compile_expression(c, e->a, sym, false), compile_expression(c, e->b, sym, false), &e->tok->loc);
			break;

		case OP_NOTEQ: {
			int temp = alloc_reg(c);
			emit_efg(c, INSTR_CMP, temp, compile_expression(c, e->a, sym, false), compile_expression(c, e->b, sym, false), &e->tok->loc);
			emit_bc(c, INSTR_FLIP, reg = alloc_reg(c), temp, &e->tok->loc);
		} break;

		case OP_LESS:
			reg = alloc_reg(c);
			emit_efg(c, INSTR_LESS, reg, compile_expression(c, e->a, sym, false), compile_expression(c, e->b, sym, false), &e->tok->loc);
			break;

		case OP_LEQ:
			reg = alloc_reg(c);
			emit_efg(c, INSTR_LEQ, reg, compile_expression(c, e->a, sym, false), compile_expression(c, e->b, sym, false), &e->tok->loc);
			break;

		case OP_MORE:
			reg = alloc_reg(c);
			emit_efg(c, INSTR_MORE, reg, compile_expression(c, e->a, sym, false), compile_expression(c, e->b, sym, false), &e->tok->loc);
			break;

		case OP_OR:
			reg = alloc_reg(c);
			emit_efg(c, INSTR_OR, reg, compile_expression(c, e->a, sym, false), compile_expression(c, e->b, sym, false), &e->tok->loc);
			break;

		case OP_COMMA:
			compile_expression(c, e->a, sym, false);
			reg = compile_expression(c, e->b, sym, true);
			break;

		case OP_POW:
			reg = alloc_reg(c);
			emit_efg(c, INSTR_POW, reg, compile_expression(c, e->a, sym, false), compile_expression(c, e->b, sym, false), &e->tok->loc);
			break;

		case OP_MODMOD: {
			reg = alloc_reg(c);
			int temp = alloc_reg(c);
			emit_efg(c, INSTR_MOD, temp,
			         compile_expression(c, e->a, sym, false),
			         compile_expression(c, e->b, sym, false),
			         &e->tok->loc);

			struct value v;
			v.type = VAL_INT;
			v.integer = 0;

			int zero = alloc_reg(c);
			emit_bc(c, INSTR_MOVC, zero, constant_table_add(c->ct, v), &e->tok->loc);

			emit_efg(c, INSTR_CMP, reg,
			         temp,
			         zero,
			         &e->tok->loc);
		} break;

		case OP_SQUIGGLEEQ: {
			int var = -1;
			reg = alloc_reg(c);

			if (e->b->type == EXPR_REGEX && e->b->val->substitution) {
				var = compile_lvalue(c, e->a, sym);
				reg = var;
				int string = alloc_reg(c);

				struct value v;
				v.type = VAL_STR;
				v.idx = gc_alloc(c->gc, VAL_STR);
				c->gc->str[v.idx] = strclone(e->b->val->substitution);
				emit_bc(c, INSTR_MOVC, string, constant_table_add(c->ct, v), &e->tok->loc);
				emit_efg(c, INSTR_SUBST, reg, compile_expression(c, e->b, sym, false), string, &e->tok->loc);
				c->code[c->ip].d.efg.h = sym->scope;
			} else {
				var = compile_expression(c, e->a, sym, false);
				emit_efg(c, INSTR_MATCH, reg, var, compile_expression(c, e->b, sym, false), &e->tok->loc);
			}
		} break;

		default:
			DOUT("unimplemented compiler for binary operator `%s'",
			     e->operator->body);
			assert(false);
		}
		break;

	case OPTYPE_PREFIX:
		switch (e->operator->name) {
		case OP_TYPE:
			reg = alloc_reg(c);
			emit_bc(c, INSTR_TYPE, reg, compile_expression(c, e->a, sym, false), &e->tok->loc);
			break;

		case OP_LENGTH:
			reg = alloc_reg(c);
			emit_bc(c, INSTR_LEN, reg, compile_expression(c, e->a, sym, false), &e->tok->loc);
			break;

		case OP_SUB:
			reg = alloc_reg(c);
			emit_bc(c, INSTR_NEG, reg, compile_expression(c, e->a, sym, false), &e->tok->loc);
			break;

		default:
			DOUT("unimplemented compiler for prefix operator `%s'",
			     e->operator->body);
			assert(false);
		}
		break;

	case OPTYPE_POSTFIX:
		switch (e->operator->name) {
		case OP_ADDADD: {
			int a = compile_lvalue(c, e->a, sym);
			reg = alloc_reg(c);
			emit_bc(c, INSTR_MOV, reg, a, &e->tok->loc);
			emit_a(c, INSTR_INC, a, &e->tok->loc);
		} break;

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
			emit_a(c, INSTR_COND, compile_expression(c, e->a, sym, false), &e->tok->loc);
			size_t a = c->ip;
			emit_d(c, INSTR_JMP, -1, &e->tok->loc);
			emit_bc(c, INSTR_MOV, reg, compile_expression(c, e->b, sym, false), &e->tok->loc);
			size_t b = c->ip;
			emit_d(c, INSTR_JMP, -1, &e->tok->loc);
			c->code[a].d.d = c->ip;
			emit_bc(c, INSTR_MOV, reg, compile_expression(c, e->c, sym, false), &e->tok->loc);
			c->code[b].d.d = c->ip;
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
			if (var->global)
				return var->address + 256;
			return var->address;
		} break;

		default:
			error_push(c->r, e->tok->loc, ERR_FATAL, "expected an lvalue");
			return -1;
		}
		break;

	case EXPR_SUBSCRIPT: {
		/* TODO: Implement global array dereferencing. */
		int reg = alloc_reg(c);
		int l = compile_lvalue(c, e->a, sym);
		emit_efg(c, INSTR_DEREF, reg,
		         l,
		         compile_expression(c, e->b, sym, false),
		         &e->tok->loc);
		return reg;
	}

	default:
		error_push(c->r, e->tok->loc, ERR_FATAL, "expected an lvalue");
		return -1;
	}

	assert(false);
}

static int
compile_expression(struct compiler *c, struct expression *e, struct symbol *sym, bool copy)
{
	int reg = -1;

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

			struct symbol *var = resolve(sym, e->val->value);
			if (var->type == SYM_FN) {
				struct value v;
				v.type = VAL_FN;
				v.num_args = var->num_arguments;
				v.integer = var->address;
				v.module = var->module->id;
				v.name = var->name;
				emit_bc(c, INSTR_MOVC, reg = alloc_reg(c), constant_table_add(c->ct, v), &e->tok->loc);
			} else if (copy) {
				emit_bc(c, INSTR_COPY, reg, var->global ? var->address + 256 : var->address, &e->tok->loc);
			} else {
				emit_bc(c, INSTR_MOV, reg, var->global ? var->address + 256 : var->address, &e->tok->loc);
			}
			break;

		default:
			reg = alloc_reg(c);
			emit_bc(c, INSTR_COPYC, reg, add_constant(c, e->val), &e->tok->loc);
			break;
		}
		break;

	case EXPR_FN_CALL:
		if (e->a->type == EXPR_VALUE && e->a->val->type == TOK_IDENTIFIER) {
			struct symbol *fs = resolve(sym, e->a->val->value);
			if (fs->type == SYM_FN && e->num != fs->num_arguments) {
				error_push(c->r, e->tok->loc, ERR_FATAL,
				           "function is called with an incorrect number of parameters");
				return -1;
			}
		}

		for (int i = e->num - 1; i >= 0; i--)
			emit_a(c, INSTR_PUSH, compile_expression(c, e->args[i], sym, true), &e->tok->loc);

		emit_a(c, INSTR_CALL, compile_expression(c, e->a, sym, false), &e->tok->loc);
		reg = alloc_reg(c);
		emit_a(c, INSTR_POP, reg, &e->tok->loc);
		break;

	case EXPR_FN_DEF: {
		struct value v;
		v.type = VAL_FN;
		v.num_args = e->s->fn_def.num;
		v.module = sym->module->id;
		v.integer = compile_statement(c, e->s);
		v.name = NULL;
		emit_bc(c, INSTR_MOVC, reg = alloc_reg(c), constant_table_add(c->ct, v), &e->tok->loc);
	} break;

	case EXPR_LIST:
		reg = alloc_reg(c);
		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(c->gc, VAL_ARRAY);
		c->gc->arrlen[v.idx] = 0;
		c->gc->array[v.idx] = NULL;

		emit_bc(c, INSTR_MOVC, reg, constant_table_add(c->ct, v), &e->tok->loc);

		for (size_t i = 0; i < e->num; i++)
			emit_bc(c, INSTR_PUSHBACK, reg, compile_expression(c, e->args[i], sym, true), &e->tok->loc);
		break;

	case EXPR_SUBSCRIPT: {
		int addr = compile_lvalue(c, e->a, sym);
		reg = alloc_reg(c);
		emit_efg(c, INSTR_SUBSCR, reg, addr, compile_expression(c, e->b, sym, false), &e->tok->loc);
	} break;

	case EXPR_MAP: {
		reg = alloc_reg(c);

		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(c->gc, VAL_ARRAY);
		c->gc->arrlen[v.idx] = 0;
		c->gc->array[v.idx] = NULL;
		emit_bc(c, INSTR_MOVC, reg, constant_table_add(c->ct, v), &e->tok->loc);

		int iter = alloc_reg(c);

		v.type = VAL_INT;
		v.integer = -1;

		int expr = compile_expression(c, e->b, sym, false);
		emit_bc(c, INSTR_MOVC, iter, constant_table_add(c->ct, v), &e->tok->loc);
		size_t start = c->ip;
		emit_a(c, INSTR_INC, iter, &e->tok->loc);
		int len = alloc_reg(c);
		emit_bc(c, INSTR_LEN, len, expr, &e->tok->loc);
		int cond = alloc_reg(c);
		emit_efg(c, INSTR_LESS, cond, iter, len, &e->tok->loc);
		emit_a(c, INSTR_COND, cond, &e->tok->loc);

		size_t a = c->ip;
		emit_d(c, INSTR_JMP, -1, &e->tok->loc);
		int temp = alloc_reg(c);
		emit_efg(c, INSTR_SUBSCR, temp, expr, iter, &e->tok->loc);
		emit_a(c, INSTR_PUSHIMP, temp, &e->tok->loc);

		int thing = compile_expression(c, e->a, sym, false);
		emit_bc(c, INSTR_PUSHBACK, reg, thing, &e->tok->loc);

		emit_a(c, INSTR_POPIMP, temp, &e->tok->loc);
		emit_d(c, INSTR_JMP, start, &e->tok->loc);
		c->code[a].d.d = c->ip;
	} break;

	case EXPR_REGEX:
		reg = alloc_reg(c);
		emit_bc(c, INSTR_MOVC, reg, add_constant(c, e->val), &e->tok->loc);
		break;

	case EXPR_GROUP: {
		reg = alloc_reg(c);
		int temp = alloc_reg(c);
		emit_bc(c, INSTR_MOVC, temp, add_constant(c, e->tok), &e->tok->loc);
		emit_bc(c, INSTR_GROUP, reg, temp, &e->tok->loc);
	} break;

	case EXPR_EVAL: {
		if (c->debug)
			fprintf(stderr, "compiling eval with %d\n", sym->scope);
		struct value v;
		v.type = VAL_INT;
		v.integer = sym->scope;
		int a = alloc_reg(c);
		emit_bc(c, INSTR_MOVC, a, constant_table_add(c->ct, v), &e->tok->loc);

		emit_efg(c, INSTR_EVAL,
		        reg = alloc_reg(c),
		        compile_expression(c, e->a, sym, false),
		        a,
		        &e->tok->loc);
	} break;

	default:
		DOUT("unimplemented compiler for expression of type `%d'",
		     e->type);
		assert(false);
	}

	return reg;
}

static int
compile_expr(struct compiler *c, struct expression *e, struct symbol *sym, bool copy)
{
	c->stack_top[c->sp] = c->stack_base[c->sp];
	return compile_expression(c, e, sym, copy);
}

#define LOOP_START(X)	  \
	do { \
		for (int i = np; i < c->np; i++) \
			c->code[c->next[i]].d.d = X; \
		c->np = np; \
	} while (0)

#define LOOP_END	  \
	do { \
		for (int i = lp; i < c->lp; i++) \
			c->code[c->last[i]].d.d = c->ip; \
		c->lp = lp; \
	} while (0)

static int
compile_statement(struct compiler *c, struct statement *s)
{
	if (!s) return -1;
	int ret = c->ip;

	struct symbol *sym = find_from_scope(c->m->sym, s->scope);
	if (c->debug) {
		fprintf(stderr, "%d> compiling %d (%s) in `%s' (%d)\n", c->sym->scope, s->type,
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
		       compile_expr(c, s->condition, sym, false), &s->tok->loc);
		start = c->ip;
		emit_d(c, INSTR_JMP, -1, &s->tok->loc);
	}

	int lp = c->lp;
	int np = c->np;

	switch (s->type) {
	case STMT_PRINTLN:
		for (size_t i = 0; i < s->print.num; i++) {
			emit_a(c, INSTR_PRINT,
			       compile_expr(c, s->print.args[i], sym, false),
			       &s->tok->loc);
		}
		emit_(c, INSTR_LINE, &s->tok->loc);
		break;

	case STMT_PRINT:
		for (size_t i = 0; i < s->print.num; i++) {
			emit_a(c, INSTR_PRINT,
			       compile_expr(c, s->print.args[i], sym, false),
			       &s->tok->loc);
		}
		break;

	case STMT_VAR_DECL:
		for (size_t i = 0; i < s->var_decl.num; i++) {
			struct symbol *var_sym = resolve(sym, s->var_decl.names[i]->value);
			var_sym->address = c->var[c->sp]++;
			int reg = -1;

			reg = s->var_decl.init
				? compile_expr(c, s->var_decl.init[i], sym, true)
				: nil(c);

			emit_bc(c, INSTR_MOV, var_sym->address, reg, &s->tok->loc);
		}
		break;

	case STMT_FN_DEF: {
		size_t a = c->ip;
		emit_d(c, INSTR_JMP, -1, &s->tok->loc);
		push_frame(c);
		sym->address = c->ip;
		ret = c->ip;

		/*
		 * TODO: think about what happens if you call a
		 * function with the wrong number of arguments.
		 */
		for (size_t i = 0; i < s->fn_def.num; i++) {
			struct symbol *arg_sym = resolve(sym, s->fn_def.args[i]->value);
			arg_sym->address = c->var[c->sp]++;
			emit_a(c, INSTR_POP, arg_sym->address, &s->tok->loc);
		}

		compile_statement(c, s->fn_def.body);
		pop_frame(c);
		emit_a(c, INSTR_PUSH, nil(c), &s->tok->loc);
		emit_(c, INSTR_RET, &s->tok->loc);
		c->code[a].d.d = c->ip;
	} break;

	case STMT_RET:
		if (c->sp == 0 && !c->eval) {
			error_push(c->r, s->tok->loc, ERR_FATAL,
			           "'return' keyword must occur inside of a function body");
		}

		emit_a(c, INSTR_PUSH, compile_expr(c, s->ret.expr, sym, true), &s->tok->loc);
		emit_(c, INSTR_RET, &s->tok->loc);
		break;

	case STMT_BLOCK:
		for (size_t i = 0; i < s->block.num; i++) {
			compile_statement(c, s->block.stmts[i]);
		}
		break;

	case STMT_EXPR:
		compile_expression(c, s->expr, sym, false);
		break;

	case STMT_IF_STMT:
		emit_a(c, INSTR_COND, compile_expr(c, s->if_stmt.cond, sym, false), &s->tok->loc);
		size_t a = c->ip;
		emit_d(c, INSTR_JMP, -1, &s->tok->loc);
		compile_statement(c, s->if_stmt.then);
		size_t b = c->ip;
		emit_d(c, INSTR_JMP, -1, &s->tok->loc);
		c->code[a].d.d = c->ip;
		compile_statement(c, s->if_stmt.otherwise);
		c->code[b].d.d = c->ip;
		break;

	case STMT_WHILE: {
		size_t a = c->ip;
		emit_a(c, INSTR_COND, compile_expr(c, s->while_loop.cond, sym, false), &s->tok->loc);
		size_t b = c->ip;
		emit_d(c, INSTR_JMP, -1, &s->tok->loc);
		compile_statement(c, s->while_loop.body);
		emit_d(c, INSTR_JMP, a, &s->tok->loc);
		c->code[b].d.d = c->ip;

		set_next(sym, a);
		set_last(sym, c->ip);
		LOOP_START(a);
		LOOP_END;
	} break;

	case STMT_DO: {
		size_t a = c->ip;
		compile_statement(c, s->do_while_loop.body);
		for (int i = np; i < c->np; i++)
			c->code[c->next[i]].d.d = c->ip;
		c->np = np;
		emit_a(c, INSTR_NCOND, compile_expr(c, s->do_while_loop.cond, sym, false), &s->tok->loc);
		emit_d(c, INSTR_JMP, a, &s->tok->loc);
		for (int i = lp; i < c->lp; i++)
			c->code[i].d.d = c->ip;
		c->lp = lp;

		set_next(sym, a);
		set_last(sym, c->ip);
	} break;

	case STMT_FOR_LOOP:
		if (s->for_loop.a && s->for_loop.b && s->for_loop.c) {
			if (s->for_loop.a->type == STMT_EXPR) {
				compile_expr(c, s->for_loop.a->expr, sym, false);
			} else if (s->for_loop.a->type == STMT_VAR_DECL) {
				compile_statement(c, s->for_loop.a);
			}

			size_t a = c->ip;
			emit_a(c, INSTR_COND, compile_expr(c, s->for_loop.b, sym, false), &s->tok->loc);
			size_t b = c->ip;
			emit_d(c, INSTR_JMP, -1, &s->tok->loc);
			compile_statement(c, s->for_loop.body);
			size_t start = c->ip;
			compile_expression(c, s->for_loop.c, sym, false);

			emit_d(c, INSTR_JMP, a, &s->tok->loc);
			c->code[b].d.d = c->ip;

			set_next(sym, start);
			LOOP_START(start);
			LOOP_END;
		}

		if (s->for_loop.a && s->for_loop.b && !s->for_loop.c) {
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
			emit_bc(c, INSTR_MOV, expr, compile_expr(c, s->for_loop.b, sym, false), &s->tok->loc);
			emit_bc(c, INSTR_MOVC, iter, constant_table_add(c->ct, v), &s->tok->loc);
			size_t start = c->ip;
			emit_a(c, INSTR_INC, iter, &s->tok->loc);
			int len = alloc_reg(c);
			emit_bc(c, INSTR_LEN, len, expr, &s->tok->loc);
			int cond = alloc_reg(c);
			emit_efg(c, INSTR_LESS, cond, iter, len, &s->tok->loc);
			emit_a(c, INSTR_COND, cond, &s->tok->loc);
			size_t a = c->ip;
			emit_d(c, INSTR_JMP, -1, &s->tok->loc);
			emit_efg(c, INSTR_SUBSCR, reg, expr, iter, &s->tok->loc);

			compile_statement(c, s->for_loop.body);
			emit_d(c, INSTR_JMP, start, &s->tok->loc);

			set_next(sym, start);
			LOOP_START(start);
			LOOP_END;
			c->code[a].d.d = c->ip;
		}

		if (s->for_loop.a && !s->for_loop.b && !s->for_loop.c) {
			if (s->for_loop.a->type != STMT_EXPR) {
				error_push(c->r, s->tok->loc, ERR_FATAL,
				           "argument to `for' must be an expression");
				return start;
			}

			int iter = c->var[c->sp]++;

			struct value v;
			v.type = VAL_INT;
			v.integer = -1;

			int expr = c->var[c->sp]++;
			emit_bc(c, INSTR_MOV, expr, compile_expr(c, s->for_loop.a->expr, sym, false), &s->tok->loc);
			emit_bc(c, INSTR_MOVC, iter, constant_table_add(c->ct, v), &s->tok->loc);
			size_t start = c->ip;
			emit_a(c, INSTR_INC, iter, &s->tok->loc);
			int len = alloc_reg(c);
			emit_bc(c, INSTR_LEN, len, expr, &s->tok->loc);
			int cond = alloc_reg(c);
			emit_efg(c, INSTR_LESS, cond, iter, len, &s->tok->loc);
			emit_a(c, INSTR_COND, cond, &s->tok->loc);
			size_t a = c->ip;
			emit_d(c, INSTR_JMP, -1, &s->tok->loc);
			int temp = alloc_reg(c);
			emit_efg(c, INSTR_SUBSCR, temp, expr, iter, &s->tok->loc);
			emit_a(c, INSTR_PUSHIMP, temp, &s->tok->loc);

			compile_statement(c, s->for_loop.body);
			emit_a(c, INSTR_POPIMP, temp, &s->tok->loc);
			emit_d(c, INSTR_JMP, start, &s->tok->loc);

			set_next(sym, start);
			LOOP_START(start);
			LOOP_END;
			c->code[a].d.d = c->ip;
		}

		set_last(sym, c->ip);
		break;

	case STMT_LAST:
		if (c->eval) {
			if (sym->last < 0) {
				error_push(c->r, s->tok->loc, ERR_FATAL,
				           "'next' keyword must occur inside of a loop body");
				return -1;
			}

			emit_d(c, INSTR_ESCAPE, sym->last, &s->tok->loc);
		} else {
			push_last(c, c->ip);
			emit_d(c, INSTR_JMP, -1, &s->tok->loc);
		}
		break;

	case STMT_NEXT:
		if (c->eval) {
			if (sym->next < 0) {
				error_push(c->r, s->tok->loc, ERR_FATAL,
				           "'next' keyword must occur inside of a loop body");
				return -1;
			}

			emit_d(c, INSTR_ESCAPE, sym->next, &s->tok->loc);
		} else {
			push_next(c, c->ip);
			emit_d(c, INSTR_JMP, -1, &s->tok->loc);
		}
		break;

	case STMT_DIE:
		emit_a(c, INSTR_KILL, compile_expression(c, s->expr, sym, false),
		       &s->tok->loc);
		break;

	case STMT_NULL:
		break;

	default:
		DOUT("unimplemented compiler for statement of type %d (%s)",
		     s->type, statement_data[s->type].body);
		assert(false);
	}

	if (s->condition)
		c->code[start].d.d = c->ip;

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
			                                       m->tree[i]->scope),
			                       true);
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
		error_write(c->r, stderr);
		free_compiler(c);
		return false;
	} else if (c->r->pending) {
		error_write(c->r, stderr);
	}

	free_compiler(c);
	return true;
}
