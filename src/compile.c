#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

#include "compile.h"
#include "tree.h"
#include "keyword.h"
#include "util.h"
#include "token.h"

static struct compiler *
new_compiler()
{
	struct compiler *c = oak_malloc(sizeof *c);

	memset(c, 0, sizeof *c);
	memset(&c->table, 0, sizeof c->table);
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
emit_instr(struct compiler *c, enum instruction_type type)
{
	emit(c, (struct instruction){type, 0, c->stmt->tok->loc});
}

static struct value
make_value(struct token *tok)
{
	struct value val;

	switch (tok->type) {
	case TOK_INTEGER:
		val.type = VAL_INT;
		val.integer = tok->integer;
		break;
	case TOK_FLOAT:
		val.type = VAL_FLOAT;
		val.real = tok->floating;
		break;
	case TOK_STRING:
		val.type = VAL_STR;
		val.str.text = tok->string;
		val.str.len = strlen(tok->string);
		break;
	default:
		DOUT("unimplemented compiler for token of type %d", tok->type);
		assert(false);
	}

	return val;
}

static size_t
add_constant(struct compiler *c, struct token *tok)
{
	c->table.val = oak_realloc(c->table.val, sizeof c->table.val[0] * (c->table.num + 1));
	c->table.val[c->table.num++] = make_value(tok);

	return c->table.num - 1;
}

static void
compile_expression(struct compiler *c, struct expression *e, struct symbol *scope)
{
	switch (e->type) {
	case EXPR_VALUE: {
		switch (e->val->type) {
		case TOK_IDENTIFIER: {
			struct symbol *sym = resolve(scope, e->val->value);
			emit(c, (struct instruction){INSTR_PUSH_LOCAL, sym->address, e->tok->loc});
		} break;

		default: {
			size_t l = add_constant(c, e->val);
			emit(c, (struct instruction){INSTR_PUSH_CONST, l, e->tok->loc});
		} break;
		}
	} break;

	case EXPR_OPERATOR:
		switch (e->operator->type) {
		case OP_BINARY: {
			compile_expression(c, e->a, scope);
			compile_expression(c, e->b, scope);

			// HACK: have enum values for the operators so this can just be a
			// switch statement.

			if (!strcmp(e->tok->value, "+")) {
				emit(c, (struct instruction){INSTR_ADD, 0, e->tok->loc});
			} else if (!strcmp(e->tok->value, "-")) {
				emit(c, (struct instruction){INSTR_SUB, 0, e->tok->loc});
			} else if (!strcmp(e->tok->value, "*")) {
				emit(c, (struct instruction){INSTR_MUL, 0, e->tok->loc});
			} else if (!strcmp(e->tok->value, "/")) {
				emit(c, (struct instruction){INSTR_DIV, 0, e->tok->loc});
			} else if (!strcmp(e->tok->value, "<")) {
				emit(c, (struct instruction){INSTR_LESS, 0, e->tok->loc});
			} else if (!strcmp(e->tok->value, "%")) {
				emit(c, (struct instruction){INSTR_MOD, 0, e->tok->loc});
			} else if (!strcmp(e->tok->value, "==")) {
				emit(c, (struct instruction){INSTR_CMP, 0, e->tok->loc});
			} else if (!strcmp(e->tok->value, "and")) {
				emit(c, (struct instruction){INSTR_AND, 0, e->tok->loc});
			} else {
				DOUT("unimplemented compiler for binary operator %s", e->tok->value);
				assert(false);
			}
		} break;
		case OP_POSTFIX: {
			compile_expression(c, e->a, scope);

			if (!strcmp(e->tok->value, "++")) {
				emit(c, (struct instruction){INSTR_INC, 0, e->tok->loc});
			} else if (!strcmp(e->tok->value, "--")) {
				emit(c, (struct instruction){INSTR_DEC, 0, e->tok->loc});
			} else {
				DOUT("unimplemented compiler for postfix operator %s", e->tok->value);
				assert(false);
			}

			// HACK: this won't work if the left-hand side is not an identifier.
			struct symbol *sym = resolve(scope, e->a->val->value);
			emit(c, (struct instruction){INSTR_POP_LOCAL, sym->address, e->tok->loc});
		} break;
		default:
			DOUT("unimplemented compiler for operator %s of type %d", e->operator->body, e->operator->type);
			assert(false);
		}
		break;
	default:
		DOUT("unimplemented compiler for expression of type %d", e->type);
		assert(false);
	}
}

static int
increase_context(struct compiler *c)
{
	c->context = realloc(c->context, sizeof c->context[0] * (c->num_contexts + 1));
	c->context[c->num_contexts++] = 0;

	return ++c->context[c->num_contexts - 1];
}

static int
get_num_variables_in_context(struct compiler *c)
{
	return c->context[c->num_contexts - 1];
}

static void
inc_num_variables_in_context(struct compiler *c)
{
	c->context[c->num_contexts - 1]++;
}

static void
compile_statement(struct compiler *c, struct statement *s)
{
//	DOUT("compiling statement of type %d (%s)", s->type, statement_data[s->type].body);
	struct symbol *sym = find_from_scope(c->sym, s->scope);

	switch (s->type) {
	case STMT_PRINT: {
		for (size_t i = 0; i < s->print.num; i++) {
			compile_expression(c, s->print.args[i], sym);
			emit_instr(c, INSTR_PRINT);
		}
	} break;
	case STMT_PRINTLN: {
		for (size_t i = 0; i < s->print.num; i++) {
			compile_expression(c, s->print.args[i], sym);
			emit_instr(c, INSTR_PRINT);
		}

		emit_instr(c, INSTR_LINE);
	} break;

	case STMT_VAR_DECL: {
		for (size_t i = 0; i < s->var_decl.num; i++) {
			struct symbol *var_sym = resolve(sym, s->var_decl.names[i]->value);
			var_sym->address = get_num_variables_in_context(c);
			inc_num_variables_in_context(c);

			compile_expression(c, s->var_decl.init[i], sym);
			/* these structures are starting to look pretty nasty.
			 * s->var_decl.init[i]->tok->loc makes a lot of sense to me,
			 * but there's probably a way to make things a bit tidier.*/
			emit(c, (struct instruction){INSTR_POP_LOCAL, var_sym->address, s->var_decl.init[i]->tok->loc});
		}
	} break;

	case STMT_BLOCK: {
		for (size_t i = 0; i < s->block.num; i++) {
			compile_statement(c, s->block.stmts[i]);
		}
	} break;

	case STMT_FOR_LOOP: {
		compile_statement(c, s->for_loop.a);
		size_t a = c->num_instr - 1;

		compile_expression(c, s->for_loop.b, sym);

		emit_instr(c, INSTR_FALSE_JUMP);
		size_t b = c->num_instr - 1;

		compile_statement(c, s->for_loop.body);
		compile_expression(c, s->for_loop.c, sym);

		c->code[b].arg = c->num_instr;

		emit(c, (struct instruction){INSTR_JUMP, a, s->for_loop.body->tok->loc});
	} break;

	case STMT_IF_STMT: {
		compile_expression(c, s->if_stmt.cond, sym);

		emit_instr(c, INSTR_FALSE_JUMP);
		size_t a = c->num_instr - 1;

		compile_statement(c, s->if_stmt.then);

		emit_instr(c, INSTR_JUMP);
		size_t b = c->num_instr - 1;

		if (s->if_stmt.otherwise) {
			c->code[a].arg = c->num_instr - 1;
			compile_statement(c, s->if_stmt.otherwise);
		} else {
			c->code[a].arg = c->num_instr - 1;
		}

		c->code[b].arg = c->num_instr - 1;
	} break;

	case STMT_EXPR: {
		compile_expression(c, s->expr, sym);
		emit_instr(c, INSTR_POP);
	} break;

	default:
		DOUT("unimplemented compiler for statement of type %d (%s)", s->type, statement_data[s->type].body);
		assert(false);
		          }
}

void
print_constant_table(FILE *f, struct constant_table table)
{
	for (size_t i = 0; i < table.num; i++) {
		struct value val = table.val[i];
		fprintf(f, "\n[%3zu %7s] ", i, value_data[val.type].body);

		if (val.type == VAL_STR) {
			print_escaped_string(f, val.str.text, val.str.len);
		} else {
			print_value(f, val);
		}
	}
}

bool
compile(struct module *m)
{
	struct compiler *c = new_compiler();
	c->sym = m->sym;
	c->num_nodes = m->num_nodes;
	increase_context(c);

	for (size_t i = 0; i < c->num_nodes; i++) {
		c->stmt = m->tree[i];
		compile_statement(c, m->tree[i]);
	}

	emit_instr(c, INSTR_END);

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
	m->constant_table = c->table;

	free_compiler(c);

	return true;
}
