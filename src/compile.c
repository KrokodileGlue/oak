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

static struct value
make_value(struct token *tok)
{
	struct value val;

	switch (tok->type) {
	case TOK_INTEGER:
		val.type = VAL_INT;
		val.integer = tok->integer;
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

static size_t
add_constant_value(struct compiler *c, struct value v)
{
	c->table.val = oak_realloc(c->table.val, sizeof c->table.val[0] * (c->table.num + 1));
	c->table.val[c->table.num++] = v;

	return c->table.num - 1;
}

static void
compile_expression(struct compiler *c, struct expression *e)
{
	switch (e->type) {
	case EXPR_VALUE: {
		switch (e->val->type) {
		case TOK_IDENTIFIER: {
			struct symbol *sym = resolve(c->si, e->val->value);
			emit(c, (struct instruction){INSTR_PUSH_LOCAL, sym->address});
		} break;
		default: {
			size_t l = add_constant(c, e->val);
			emit(c, (struct instruction){INSTR_PUSH_CONST, l});
		} break;
		}
	} break;

	case EXPR_OPERATOR:
		switch (e->operator->type) {
		case OP_BINARY: {
			compile_expression(c, e->a);
			compile_expression(c, e->b);

			// HACK: have enum values for the operators so this can just be a
			// switch statement.

			if (!strcmp(e->tok->value, "+")) {
				emit(c, (struct instruction){INSTR_ADD, 0});
			} else if (!strcmp(e->tok->value, "-")) {
				emit(c, (struct instruction){INSTR_SUB, 0});
			} else if (!strcmp(e->tok->value, "*")) {
				emit(c, (struct instruction){INSTR_MUL, 0});
			} else if (!strcmp(e->tok->value, "/")) {
				emit(c, (struct instruction){INSTR_DIV, 0});
			}
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

static void
compile_statement(struct compiler *c, struct statement *s)
{
//	DOUT("compiling statement of type %d (%s)", s->type, statement_data[s->type].body);

	switch (s->type) {
	case STMT_PRINT: {
		for (size_t i = 0; i < s->print.num; i++) {
			compile_expression(c, s->print.args[i]);
			emit(c, (struct instruction){INSTR_PRINT, 0});
		}
	} break;
	case STMT_PRINTLN: {
		for (size_t i = 0; i < s->print.num; i++) {
			compile_expression(c, s->print.args[i]);
			emit(c, (struct instruction){INSTR_PRINT, 0});
		}

		struct value newline;
		newline.type = VAL_STR;
		newline.str.text = strclone("\n");
		newline.str.len = strlen(newline.str.text);
		size_t nl = add_constant_value(c, newline);
		emit(c, (struct instruction){INSTR_PUSH_CONST, nl});
		emit(c, (struct instruction){INSTR_PRINT, 0});
	} break;

	case STMT_VAR_DECL: {
		for (size_t i = 0; i < s->var_decl.num; i++) {
			struct symbol *sym = resolve(c->si, s->var_decl.names[i]->value);
			sym->address = i;
		}

		for (size_t i = 0; i < s->var_decl.num; i++) {
			struct symbol *sym = resolve(c->si, s->var_decl.names[i]->value);
			compile_expression(c, s->var_decl.init[i]);
			emit(c, (struct instruction){INSTR_POP_LOCAL, sym->address});
		}
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
		fprintf(f, "\nconstant %zu (%s):", i, value_data[val.type].body);

		switch (val.type) {
		case VAL_INT:
			fprintf(f, "%"PRId64, val.integer);
			break;
		case VAL_STR:
			fprintf(f, "%s", val.str.text);
			break;
		default:
			DOUT("unimplemented printer for constant");
			assert(false);
		}
	}
}

bool
compile(struct module *m, struct oak *k)
{
	struct symbolizer *si = new_symbolizer(k);
	struct compiler *c = new_compiler();
	si->symbol = m->sym;
	c->si = si;
	c->num_nodes = m->num_nodes;

	for (size_t i = 0; i < c->num_nodes; i++) {
		compile_statement(c, m->tree[i]);
	}

	emit(c, (struct instruction){INSTR_END, 0});

	m->code = c->code;
	m->num_instr = c->num_instr;
	m->stage = MODULE_STAGE_COMPILED;
	m->constant_table = c->table;

	free_compiler(c);

	return true;
}
