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
#include "parser.h"

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
	free(c->var);
	error_clear(c->r);
	free(c);
}

static void
emit(struct compiler *c, struct instruction instr)
{
	c->code = realloc(c->code, sizeof c->code[0] * (c->ip + 1));
	instr.loc = &c->stmt->tok->loc;
	c->code[c->ip++] = instr;
}

static void
emit_(struct compiler *c, enum instruction_type type)
{
	emit(c, (struct instruction){type, .d = { 0 }});
}

static void
emit_a(struct compiler *c, enum instruction_type type, uint8_t a)
{
	emit(c, (struct instruction){type, .d = { a }});
}

static void
emit_bc(struct compiler *c, enum instruction_type type, uint8_t B, uint8_t C)
{
	emit(c, (struct instruction){type, .d = { .bc = {B, C} }});
}

static void
emit_efg(struct compiler *c, enum instruction_type type, uint8_t E, uint8_t F, uint8_t G)
{
	emit(c, (struct instruction){type, .d = { .efg = {E, F, G} }});
}

static void
push_frame(struct compiler *c)
{
	c->stack_top = oak_realloc(c->stack_top, (c->sp + 2)
	                           * sizeof *c->stack_top);
	c->var = oak_realloc(c->var, (c->sp + 2)
	                           * sizeof *c->var);
	c->sp++;
	c->var[c->sp] = 0;
	c->stack_top[c->sp] = c->sym->num_variables;
}

static void
pop_frame(struct compiler *c)
{
	c->sp--;
}

static int
alloc_reg(struct compiler *c)
{
	/* TODO: make sure we have enough registers n stuff. */
	/* TODO: put a cap on the recursion */
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

	default:
		DOUT("unimplemented token-to-value converter");
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
	emit_bc(c, INSTR_MOVC, reg, n);
	return reg;
}

static int compile_expression(struct compiler *c, struct expression *e, struct symbol *sym);
static int compile_lvalue(struct compiler *c, struct expression *e, struct symbol *sym);

static int
compile_operator(struct compiler *c, struct expression *e, struct symbol *sym)
{
	int reg = -1;

#define o(X)	  \
	case OP_##X: \
		emit_efg(c, INSTR_##X, reg = alloc_reg(c), \
		         compile_expression(c, e->a, sym), \
		         compile_expression(c, e->b, sym)); \
		break

	switch (e->operator->type) {
	case OPTYPE_BINARY:
		switch (e->operator->name) {
			o(ADD);
			o(MUL);
			o(DIV);
			o(SUB);

		case OP_EQ:
			if (e->a->type == EXPR_SUBSCRIPT) {
				emit_efg(c, INSTR_ASET,
				         compile_lvalue(c, e->a->a, sym),
				         compile_expression(c, e->a->b, sym),
				         compile_expression(c, e->b, sym));

			} else {
				emit_bc(c, INSTR_MOV, compile_lvalue(c, e->a, sym),
				        compile_expression(c, e->b, sym));
			}
			break;

		default:
			DOUT("unimplemented operator compiler for binary operator `%s'",
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
			return var->address;
		} break;

		default:
			error_push(c->r, e->tok->loc, ERR_FATAL, "expected an lvalue");
			return -1;
		}
		break;

	case EXPR_SUBSCRIPT: {
		int reg = alloc_reg(c);
		emit_efg(c, INSTR_DEREF, reg,
		         compile_lvalue(c, e->a, sym),
		         compile_expression(c, e->b, sym));
		return reg;
	} break;

	default:
		error_push(c->r, e->tok->loc, ERR_FATAL, "expected an lvalue");
		return -1;
	}

	assert(false);
}

static int
compile_expression(struct compiler *c, struct expression *e, struct symbol *sym)
{
	int reg = -1;

	switch (e->type) {
	case EXPR_VALUE:
		switch (e->val->type) {
		case TOK_IDENTIFIER:
			reg = alloc_reg(c);
			emit_bc(c, INSTR_MOV, reg, resolve(sym, e->val->value)->address);
			break;

		default:
			reg = alloc_reg(c);
			emit_bc(c, INSTR_MOVC, reg, add_constant(c, e->val));
			break;
		}
		break;

	case EXPR_OPERATOR:
		reg = compile_operator(c, e, sym);
		break;

	case EXPR_FN_CALL:
		for (size_t i = 0; i < e->num; i++) {
			emit_a(c, INSTR_PUSH, compile_expression(c, e->args[i], sym));
		}

		emit_a(c, INSTR_CALL, compile_lvalue(c, e->a, sym));
		reg = alloc_reg(c);
		emit_a(c, INSTR_POP, reg);
		break;

	case EXPR_LIST:
		reg = alloc_reg(c);
		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(c->gc, VAL_ARRAY);
		c->gc->arrlen[v.idx] = 0;
		c->gc->array[v.idx] = NULL;

		emit_bc(c, INSTR_MOVC, reg, constant_table_add(c->ct, v));

		for (size_t i = 0; i < e->num; i++)
			emit_bc(c, INSTR_PUSHBACK, reg, compile_expression(c, e->args[i], sym));
		break;

	default:
		DOUT("unimplemented compiler for expression of type `%d'",
		     e->type);
		assert(false);
	}

	return reg;
}

static int
compile_expr(struct compiler *c, struct expression *e, struct symbol *sym)
{
	int a = compile_expression(c, e, sym);
	c->stack_top[c->sp] = sym->num_variables;
	return a;
}

static void
compile_statement(struct compiler *c, struct statement *s)
{
	struct symbol *sym = find_from_scope(c->sym, s->scope);
	if (c->debug) {
		fprintf(stderr, "compiling %d (%s) in `%s'\n", s->type,
		        statement_data[s->type].body, sym->name);
	}

	switch (s->type) {
	case STMT_PRINTLN:
		for (size_t i = 0; i < s->print.num; i++)
			emit_a(c, INSTR_PRINT,
			       compile_expr(c, s->print.args[i], sym));
		emit_(c, INSTR_LINE);
		break;

	case STMT_PRINT:
		for (size_t i = 0; i < s->print.num; i++)
			emit_a(c, INSTR_PRINT,
			       compile_expr(c, s->print.args[i], sym));
		break;

	case STMT_VAR_DECL:
		for (size_t i = 0; i < s->var_decl.num; i++) {
			struct symbol *var_sym = resolve(sym, s->var_decl.names[i]->value);
			var_sym->address = c->var[c->sp]++;
			int reg = -1;

			reg = s->var_decl.init
				? compile_expr(c, s->var_decl.init[i], sym)
				: nil(c);

			emit_bc(c, INSTR_MOV, var_sym->address, reg);
		}
		break;

	case STMT_FN_DEF: {
		size_t a = c->ip;
		emit_a(c, INSTR_JMP, 0);
		push_frame(c);
		sym->address = c->ip;

		/*
		 * TODO: think about what happens if you call a
		 * function with the wrong number of arguments.
		 */
		for (size_t i = 0; i < s->fn_def.num; i++) {
			struct symbol *arg_sym = resolve(sym, s->fn_def.args[i]->value);
			arg_sym->address = c->var[c->sp]++;
			emit_a(c, INSTR_POP, arg_sym->address);
		}

		compile_statement(c, s->fn_def.body);
		pop_frame(c);
		emit_a(c, INSTR_PUSH, nil(c));
		emit_(c, INSTR_RET);
		c->code[a].d.a = c->ip;
	} break;

	case STMT_RET:
		if (c->sp == 0) {
			error_push(c->r, s->tok->loc, ERR_FATAL,
			           "return statement occurs outside of function body");
		}

		emit_a(c, INSTR_PUSH, compile_expr(c, s->ret.expr, sym));
		emit_(c, INSTR_RET);
		break;

	case STMT_BLOCK:
		for (size_t i = 0; i < s->block.num; i++) {
			compile_statement(c, s->block.stmts[i]);
		}
		break;

	case STMT_EXPR:
		compile_expression(c, s->expr, sym);
		break;

	default:
		DOUT("unimplemented compiler for statement of type %d (%s)",
		     s->type, statement_data[s->type].body);
		assert(false);
	}
}

bool
compile(struct module *m, bool debug)
{
	struct compiler *c = new_compiler();
	c->sym = m->sym;
	c->num_nodes = m->num_nodes;
	c->stmt = m->tree[0];
	c->ct = new_constant_table();
	c->gc = m->gc;
	c->debug = debug;
	c->sp = -1;
	push_frame(c);

	for (size_t i = 0; i < c->num_nodes; i++) {
		c->stmt = m->tree[i];
		compile_statement(c, m->tree[i]);
	}

	emit_(c, INSTR_END);

	if (c->r->fatal) {
		error_write(c->r, stderr);
		free_compiler(c);
		return false;
	} else if (c->r->pending) {
		error_write(c->r, stderr);
	}

	m->code = c->code;
	m->num_instr = c->ip;
	m->stage = MODULE_STAGE_COMPILED;
	m->ct = c->ct;

	free_compiler(c);

	return true;
}
