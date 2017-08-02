#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

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
	case TOK_BOOL:
		val.type = VAL_BOOL;
		val.boolean = tok->boolean;
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
push_integer(struct compiler *c, int64_t integer)
{
	struct value val;
	val.type = VAL_INT;
	val.integer = integer;
	size_t addr = add_constant_value(c, val);
	emit(c, (struct instruction){INSTR_PUSH_CONST, addr, c->stmt->tok->loc});
}

static void
push_nil(struct compiler *c)
{
	struct value val;
	val.type = VAL_NIL;
	size_t addr = add_constant_value(c, val);
	emit(c, (struct instruction){INSTR_PUSH_CONST, addr, c->stmt->tok->loc});
}

static void compile_expression(struct compiler *c, struct expression *e, struct symbol *scope);

static void
compile_lvalue(struct compiler *c, struct expression *e, struct symbol *scope, int subscript)
{
	switch (e->type) {
	case EXPR_VALUE:
		switch (e->val->type) {
		case TOK_IDENTIFIER: {
			struct symbol *var = resolve(scope, e->val->value);
			push_integer(c, subscript);
			push_integer(c, var->address);
		} break;
		default:
			error_push(c->r, e->tok->loc, ERR_FATAL, "expected an lvalue");
			break;
		}
		break;

	case EXPR_SUBSCRIPT:
		compile_expression(c, e->b, scope);
		compile_lvalue(c, e->a, scope, subscript + 1);
		break;

	default:
		error_push(c->r, e->tok->loc, ERR_FATAL, "expected an lvalue");
		break;
	}
}

static void
push_string(struct compiler *c, char *str)
{
	struct value val;
	val.type = VAL_STR;
	val.str.text = strclone(str);
	val.str.len = strlen(str);

	size_t addr = add_constant_value(c, val);
	emit(c, (struct instruction){INSTR_PUSH_CONST, addr, c->stmt->tok->loc});
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

		case TOK_STRING: {
			if (e->tok->is_interpolatable) {
				size_t start = 0, i, num = 0;

				for (i = 0; i < strlen(e->tok->string); i++) {
					if (e->tok->string[i] == '{' && (!i ? true : e->tok->string[i - 1] != '\\')) {
						num++;

						char *thing = substr(e->tok->string, start, i);
						push_string(c, thing);
						i++;
						start = i;

						while (e->tok->string[i] != '}' && e->tok->string[i]) i++;
						if (!e->tok->string[i]) {
							error_push(c->r, e->tok->loc, ERR_FATAL, "unterminated { in interpolated string");
							break;
						}

						char *other = substr(e->tok->string, start, i);
						struct token *tok = lex_isolated(other, "*interpolate*");
						struct expression *expr = parse_isolated_expr(tok);
						compile_expression(c, expr, scope);
						emit(c, (struct instruction){INSTR_ADD, 0, tok->loc});

						i++;
						start = i;
					}

					if (i && e->tok->string[i - 1] == '\\' && e->tok->string[i] == '{') {
						remove_char(e->tok->string, i - 1);
					}
				}

				if (num)
					for (size_t i = 0; i < num - 1; i++)
						emit_instr(c, INSTR_ADD);

				push_string(c, substr(e->tok->string, start, i));
				if (num) emit_instr(c, INSTR_ADD);
			} else {
				size_t l = add_constant(c, e->val);
				emit(c, (struct instruction){INSTR_PUSH_CONST, l, e->tok->loc});
			}
		} break;

		default: {
			size_t l = add_constant(c, e->val);
			emit(c, (struct instruction){INSTR_PUSH_CONST, l, e->tok->loc});
		} break;
		}
	} break;

	case EXPR_OPERATOR:
		switch (e->operator->type) {
		case OPTYPE_BINARY: {
			if (e->operator->name == OP_COMMA) {
				compile_expression(c, e->a, scope);
				emit_instr(c, INSTR_POP);
				compile_expression(c, e->b, scope);
				break;
			} else if (e->operator->name == OP_EQ) {
				compile_lvalue(c, e->a, scope, 0);
				compile_expression(c, e->b, scope);
				emit_instr(c, INSTR_ASSIGN);
				break;
			}

			compile_expression(c, e->a, scope);
			compile_expression(c, e->b, scope);

#define o(x) \
	emit(c, (struct instruction){INSTR_##x, 0, e->tok->loc});
			switch (e->operator->name) {
			case OP_ADD:   o(ADD)                                break;
			case OP_SUB:   o(SUB)                                break;
			case OP_MUL:   o(MUL)                                break;
			case OP_DIV:   o(DIV)                                break;
			case OP_LESS:  o(LESS)                               break;
			case OP_LEQ:   o(LEQ)                                break;
			case OP_MORE:  o(MORE)                               break;
			case OP_MOD:   o(MOD)                                break;
			case OP_MODMOD:o(MOD) push_integer(c, 0); o(CMP)     break;
			case OP_EQEQ:  o(CMP)                                break;
			case OP_AND:   o(AND)                                break;
			case OP_OR:    o(OR)                                 break;
			case OP_NOR:   o(OR) o(FLIP)                         break;
			case OP_NOTEQ: o(CMP) o(FLIP)                        break;
			case OP_EXCLAMATION: o(FLIP)                         break;
			case OP_SQUIGGLE_ARROW: push_integer(c, 1); o(RANGE) break;
			default: {
				DOUT("internal error; an operator of name %d was encountered", e->operator->name);
				assert(false);
			} break;
			}
		} break;
		case OPTYPE_POSTFIX: {
			compile_expression(c, e->a, scope);

			// HACK: this won't work if the left-hand side is not an identifier.
			struct symbol *sym = resolve(scope, e->a->val->value);

			if (!strcmp(e->tok->value, "++")) {
				compile_expression(c, e->a, scope);
				emit(c, (struct instruction){INSTR_INC, 0, e->tok->loc});
				emit(c, (struct instruction){INSTR_POP_LOCAL, sym->address, e->tok->loc});
			} else if (!strcmp(e->tok->value, "--")) {
				compile_expression(c, e->a, scope);
				emit(c, (struct instruction){INSTR_DEC, 0, e->tok->loc});
				emit(c, (struct instruction){INSTR_POP_LOCAL, sym->address, e->tok->loc});
			} else {
				DOUT("unimplemented compiler for postfix operator %s", e->tok->value);
				assert(false);
			}
		} break;
		case OPTYPE_PREFIX: {
			compile_expression(c, e->a, scope);

			switch (e->operator->name) {
			case OP_LENGTH: o(LEN); break;
			case OP_SAY: o(SAY); break;
			case OP_SAYLN: o(SAY); emit_instr(c, INSTR_LINE); break;
			case OP_TYPE: o(TYPE); break;
			case OP_SUB: o(NEG); break;
			default: {
				DOUT("internal error; an operator of name %d was encountered", e->operator->name);
				assert(false);
			} break;
			}
		} break;
		default:
			DOUT("unimplemented compiler for operator %s of type %d", e->operator->body, e->operator->type);
			assert(false);
		}
		break;

	case EXPR_LIST: {
		for (int i = e->num - 1 ? e->num - 1 : 0; i >= 0; i--) {
			compile_expression(c, e->args[i], scope);

//			uint64_t h = hash((char *)&i, sizeof i);

			/* the key for each value will just be it's index in the array. */
			struct value val = (struct value){VAL_INT, { i } };
			size_t v = add_constant_value(c, val);
			emit(c, (struct instruction){INSTR_PUSH_CONST, v, e->args[i]->tok->loc});
		}

		/* push the number of elements onto the stack */
		struct value val = (struct value){VAL_INT, { (uint64_t)e->num }};
		size_t v = add_constant_value(c, val);
		emit(c, (struct instruction){INSTR_PUSH_CONST, v, e->tok->loc});

		emit_instr(c, INSTR_LIST);
	} break;

	case EXPR_SUBSCRIPT: {
		compile_expression(c, e->a, scope);
		compile_expression(c, e->b, scope);
		emit_instr(c, INSTR_SUBSCRIPT);
	} break;

	case EXPR_FN_CALL: {
		for (size_t i = 0; i < e->num; i++) {
			compile_expression(c, e->args[i], scope);
		}

		compile_lvalue(c, e->a, scope, 0);
		emit_instr(c, INSTR_CALL);
	} break;

	default:
		DOUT("unimplemented compiler for expression of type %d", e->type);
		assert(false);
	}
}

#undef o

static int
increase_context(struct compiler *c)
{
	c->context = realloc(c->context, sizeof c->context[0] * (c->num_contexts + 1));
	c->context[c->num_contexts++] = 0;

	return c->context[c->num_contexts - 1];
}

static int
get_num_variables_in_context(struct compiler *c)
{
	return c->context[c->num_contexts - 1];
}

static void
inc_num_variables_in_context(struct compiler *c)
{
	c->context[c->num_contexts - 1] += 1;
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

			if (s->var_decl.init)
				compile_expression(c, s->var_decl.init[i], sym);
			else
				push_nil(c);
			/*
			 * these structures are starting to look pretty nasty.
			 * s->var_decl.init[i]->tok->loc makes a lot of sense to me,
			 * but there's probably a way to make things a bit tidier.
			 */
			emit(c, (struct instruction){INSTR_POP_LOCAL, var_sym->address,
						s->var_decl.init ? s->var_decl.init[i]->tok->loc : s->var_decl.names[i]->loc});
		}
	} break;

	case STMT_BLOCK: {
		for (size_t i = 0; i < s->block.num; i++) {
			compile_statement(c, s->block.stmts[i]);
		}
	} break;

	case STMT_FOR_LOOP: {
		if (s->for_loop.a && s->for_loop.b && s->for_loop.c) {
			compile_statement(c, s->for_loop.a);

			size_t a = c->num_instr;
			compile_expression(c, s->for_loop.b, sym);

			size_t b = c->num_instr;
			emit_instr(c, INSTR_FALSE_JUMP);

			compile_statement(c, s->for_loop.body);
			compile_expression(c, s->for_loop.c, sym);

			c->code[b].arg = c->num_instr;

			emit(c, (struct instruction){INSTR_JUMP, a, s->for_loop.body->tok->loc});
		} else if (s->for_loop.a && s->for_loop.b) {
			compile_statement(c, s->for_loop.a);
			size_t iterator = get_num_variables_in_context(c);
			inc_num_variables_in_context(c);

			compile_expression(c, s->for_loop.b, sym);
			emit(c, (struct instruction){INSTR_MKITER, 0, s->for_loop.b->tok->loc});
			emit(c, (struct instruction){INSTR_POP_LOCAL, iterator, s->for_loop.b->tok->loc});

			size_t a = c->num_instr;
			emit(c, (struct instruction){INSTR_ITER, iterator, s->for_loop.b->tok->loc});
			struct symbol *var_sym = resolve(sym, s->for_loop.a->var_decl.names[0]->value);
			emit(c, (struct instruction){INSTR_POP_LOCAL, var_sym->address, s->for_loop.b->tok->loc});

			emit(c, (struct instruction){INSTR_PUSH_LOCAL, var_sym->address, s->for_loop.b->tok->loc});
			push_nil(c);
			emit(c, (struct instruction){INSTR_CMP, 0, s->for_loop.b->tok->loc});
			size_t b = c->num_instr;
			emit(c, (struct instruction){INSTR_TRUE_JUMP, 0, s->for_loop.b->tok->loc});

			compile_statement(c, s->for_loop.body);
			emit(c, (struct instruction){INSTR_JUMP, a, s->for_loop.b->tok->loc});
			c->code[b].arg = c->num_instr - 1;
		} else {
			struct symbol *var_sym = resolve(sym, "i");

			size_t iterator = get_num_variables_in_context(c);
			inc_num_variables_in_context(c);
			var_sym->address = get_num_variables_in_context(c);
			inc_num_variables_in_context(c);

			compile_expression(c, s->for_loop.a->expr, sym);
			emit(c, (struct instruction){INSTR_MKITER, 0, s->for_loop.a->tok->loc});
			emit(c, (struct instruction){INSTR_POP_LOCAL, iterator, s->for_loop.a->tok->loc});

			size_t a = c->num_instr;
			emit(c, (struct instruction){INSTR_ITER, iterator, s->for_loop.a->tok->loc});
			emit(c, (struct instruction){INSTR_POP_LOCAL, var_sym->address, s->for_loop.a->tok->loc});

			emit(c, (struct instruction){INSTR_PUSH_LOCAL, var_sym->address, s->for_loop.a->tok->loc});
			push_nil(c);
			emit(c, (struct instruction){INSTR_CMP, 0, s->for_loop.a->tok->loc});
			size_t b = c->num_instr;
			emit(c, (struct instruction){INSTR_TRUE_JUMP, 0, s->for_loop.a->tok->loc});

			compile_statement(c, s->for_loop.body);
			emit(c, (struct instruction){INSTR_JUMP, a, s->for_loop.a->tok->loc});
			c->code[b].arg = c->num_instr - 1;
		}
	} break;

	case STMT_WHILE: {
		size_t a = c->num_instr;
		compile_expression(c, s->while_loop.cond, sym);

		size_t b = c->num_instr;
		emit(c, (struct instruction){INSTR_FALSE_JUMP, 0, s->while_loop.cond->tok->loc});

		compile_statement(c, s->while_loop.body);
		emit(c, (struct instruction){INSTR_JUMP, a, s->while_loop.cond->tok->loc});

		c->code[b].arg = c->num_instr - 1;
	} break;

	/* note to self: be super duper careful about editing previous instructions. */
	case STMT_IF_STMT: {
		compile_expression(c, s->if_stmt.cond, sym);

		size_t a = c->num_instr;
		emit_instr(c, INSTR_FALSE_JUMP);

		compile_statement(c, s->if_stmt.then);

		size_t b = c->num_instr;
		emit_instr(c, INSTR_JUMP);

		c->code[a].arg = c->num_instr - 1;
		if (s->if_stmt.otherwise)
			compile_statement(c, s->if_stmt.otherwise);

		c->code[b].arg = c->num_instr;
	} break;

	case STMT_FN_DEF: {
		size_t a = c->num_instr;
		emit(c, (struct instruction){INSTR_JUMP, 0, s->tok->loc});
		increase_context(c);

		sym->address = c->num_instr;
		push_integer(c, sym->num_variables);
		emit(c, (struct instruction){INSTR_FRAME, sym->num_variables, s->tok->loc});

		for (size_t i = 0; i < s->fn_def.num; i++) {
			struct symbol *arg_sym = resolve(sym, s->fn_def.args[i]->value);
			arg_sym->address = get_num_variables_in_context(c);
			inc_num_variables_in_context(c);

			emit(c, (struct instruction){INSTR_POP_LOCAL, arg_sym->address, s->fn_def.args[i]->loc});
		}

		compile_statement(c, s->fn_def.body);

		push_nil(c);
		emit(c, (struct instruction){INSTR_RET, 0, s->tok->loc});
		c->code[a].arg = c->num_instr;
	} break;

	case STMT_EXPR: {
		compile_expression(c, s->expr, sym);
		emit_instr(c, INSTR_POP);
	} break;

	case STMT_RET: {
		compile_expression(c, s->expr, sym);
		emit_instr(c, INSTR_RET);
	} break;

	case STMT_NULL: break;

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
	c->stmt = m->tree[0];
	increase_context(c);

	push_integer(c, c->sym->num_variables);
	emit(c, (struct instruction){INSTR_FRAME, 0, m->tree[0]->tok->loc});

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
