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
	free(c);
}

static void
emit(struct compiler *c, struct instruction instr)
{
	c->code = realloc(c->code, sizeof c->code[0] * (c->num_instr + 1));
	c->code[c->num_instr++] = instr;
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

static int
alloc_reg(struct compiler *c)
{
	/* TODO: make sure we have enough registers n stuff. */
	c->stack_top = realloc(c->stack_top, (c->sp + 1) * sizeof *c->stack_top);
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
compile_expression(struct compiler *c, struct expression *e, struct symbol *sym)
{
	int reg = 0;

	switch (e->type) {
	case EXPR_VALUE:
		switch (e->val->type) {
		default:
			reg = alloc_reg(c);
			emit_bc(c, INSTR_MOVC, reg, add_constant(c, e->val));
			break;
		}
		break;
	default:
		assert(false);
	}

	return reg;
}

static void
compile_statement(struct compiler *c, struct statement *s)
{
	if (c->debug)
		DOUT("compiling statement of type %d (%s)", s->type,
		     statement_data[s->type].body);

	struct symbol *sym = c->sym;

	switch (s->type) {
	case STMT_PRINTLN:
		for (size_t i = 0; i < s->print.num; i++)
			emit_a(c, INSTR_PRINT,
			       compile_expression(c, s->print.args[i], sym));
		emit_(c, INSTR_LINE);
		break;

	case STMT_PRINT:
		for (size_t i = 0; i < s->print.num; i++)
			emit_a(c, INSTR_PRINT,
			       compile_expression(c, s->print.args[i], sym));
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
	// m->tree[0]->tok->loc
	/* emit(c, (struct instruction){INSTR_FRAME, .d = {{0,0,0}}}); */

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
	m->num_instr = c->num_instr;
	m->stage = MODULE_STAGE_COMPILED;
	m->ct = c->ct;

	free_compiler(c);

	return true;
}
