#include "parse.h"
#include "error.h"
#include "util.h"
#include "operator.h"
#include "color.h"
#include "module.h"
#include "builtin.h"

#include <stdbool.h>
#include <assert.h>

#define NEXT if (ps->tok->type != TOK_END) ps->tok = ps->tok->next;

struct parser *
new_parser()
{
	struct parser *ps = oak_malloc(sizeof *ps);
	ps->r = new_reporter();
	return ps;
}

void
free_parser(struct parser *ps)
{
	error_clear(ps->r);
	/* the parser is not responsible for the token stream, so we
	 * don't have to free it here. */
	free(ps);
}

static void
expect_symbol(struct parser *ps, char *sym)
{
//	DOUT("expecting %s, looking at %s\n", sym, ps->tok->value);

	if (strcmp(ps->tok->value, sym)) {
		if (ps->tok->type == TOK_END) {
			struct token *tok = ps->tok->prev;

			error_push(ps->r, tok->loc, ERR_FATAL,
				   "unexpected end-of-file; expected '" ERROR_LOCATION_COLOR "%s" RESET_COLOR "'",
				   sym);
		} else {
			error_push(ps->r, ps->tok->loc, ERR_FATAL,
				   "unexpected token '"
				   ERROR_LOCATION_COLOR "%s" RESET_COLOR
				   "'; expected '"
				   ERROR_LOCATION_COLOR "%s" RESET_COLOR "'",
				   ps->tok->value, sym);
		}
	}

	NEXT;
}

static void
expect_terminator(struct parser *ps)
{
	if (strcmp(ps->tok->value, ";") && strcmp(ps->tok->value, "}") && strcmp(ps->tok->value, "when") && !ps->tok->prev->is_line_end) {
		if (ps->tok->type == TOK_END) {
			struct token *tok = ps->tok->prev;

			error_push(ps->r, tok->loc, ERR_FATAL,
				   "unexpected end-of-file; expected an expression terminator");
		} else {
			error_push(ps->r, ps->tok->loc, ERR_FATAL,
				   "unexpected token '"
				   ERROR_LOCATION_COLOR "%s" RESET_COLOR
				   "'; expected an expression terminator",
				   ps->tok->value);
		}
	}

	if (!strcmp(ps->tok->value, ";")) NEXT;
}

static struct statement *parse_stmt(struct parser *ps);
static struct statement *parse_vardecl(struct parser *ps);
static struct statement *parse_fn_def(struct parser *ps);
static struct expression *parse_expression(struct parser *ps, int prec);

static struct expression *
parse_table(struct parser *ps)
{
	struct expression *e = new_expression(ps->tok);
	e->type = EXPR_TABLE;
	expect_symbol(ps, "{");

	while (ps->tok->type == TOK_STRING
	       || ps->tok->type == TOK_IDENTIFIER) {
		e->keys = oak_realloc(e->keys, (e->num + 1) * sizeof *e->keys);
		e->args = oak_realloc(e->args, (e->num + 1) * sizeof *e->args);

		e->keys[e->num] = ps->tok;
		NEXT;

		if (!strcmp(ps->tok->value, "=")) {
			expect_symbol(ps, "=");
			e->args[e->num] = parse_expression(ps, 1);
		} else {
			e->args[e->num] = NULL;
		}

		e->num++;

		if (!strcmp(ps->tok->value, "}")) {
			break;
		} else {
			if (strcmp(ps->tok->value, ",")) {
				expect_symbol(ps, ",");
				NEXT;
				break;
			}

			expect_symbol(ps, ",");
		}
	}

	expect_symbol(ps, "}");
	return e;
}

static struct operator *
get_infix_op(struct parser *ps)
{
	for (size_t i = 0; i < num_ops(); i++) {
		if (!strcmp(ops[i].body, ps->tok->value)
		    && ops[i].type != OPTYPE_PREFIX) {
			return ops + i;
		}
	}

	return NULL;
}

static struct operator *
get_prefix_op(struct parser *ps)
{
	for (size_t i = 0; i < num_ops(); i++) {
		if (!strcmp(ops[i].body, ps->tok->value)
		    && ops[i].type == OPTYPE_PREFIX) {
			return ops + i;
		}
	}

	return NULL;
}

static size_t
get_prec(struct parser *ps, size_t prec)
{
	if (get_infix_op(ps))
		return get_infix_op(ps)->prec;

	return prec;
}

static struct expression *
parse_expr(struct parser *ps, size_t prec)
{
	struct expression *left = new_expression(ps->tok);
	struct operator *op = get_prefix_op(ps);

	if (!strcmp(ps->tok->value, "fn")) {
		left->type = EXPR_FN_DEF;
		left->s = parse_fn_def(ps);
	} else if (op || !strcmp(ps->tok->value, "(")) {
		left->type = EXPR_OPERATOR;
		left->operator = op;

		NEXT;
		if (op) {
			left->a = parse_expr(ps, op->prec);
		} else { /* otherwise we're looking at a '(' */
			free(left);
			left = parse_expr(ps, 0);
			expect_symbol(ps, ")");
		}
	} else if (!strcmp(ps->tok->value, "[")) {
		left->type = EXPR_LIST;
		NEXT;
		if (!strcmp(ps->tok->value, "]")) {
			expect_symbol(ps, "]"); /* we have an empty list */
		} else {
			struct expression *first = parse_expr(ps, 1);
			if (!strcmp(ps->tok->value, "for")) {
				/* List comprehension. */
				left->type = EXPR_LIST_COMPREHENSION;
				left->a = first;

				expect_symbol(ps, "for");

				if (!strcmp(ps->tok->value, "var")) {
					left->s = parse_vardecl(ps);
				} else {
					left->s = new_statement(ps->tok);
					left->s->type = STMT_EXPR;
					left->s->expr = parse_expr(ps, 0);
				}

				if (!strcmp(ps->tok->value, "in")) {
					expect_symbol(ps, "in");

					left->b = parse_expr(ps, 0);
					if (!strcmp(ps->tok->value, "if")) {
						expect_symbol(ps, "if");
						left->c = parse_expr(ps, 0);
					}
				}
			} else {
				if (strcmp(ps->tok->value, "]")) NEXT;
				left->args = oak_realloc(left->args, (left->num + 1) * sizeof *left->args);
				left->args[left->num++] = first;
				while (strcmp(ps->tok->value, "]")) {
					left->args = oak_realloc(left->args, sizeof left->args[0] * (left->num + 1));
					left->args[left->num++] = parse_expr(ps, 1);
					if (!strcmp(ps->tok->value, ",")) NEXT;
				}
			}

			expect_symbol(ps, "]");
		}
	} else if (!strcmp(ps->tok->value, "map")) {
		left->type = EXPR_MAP;
		NEXT;
		expect_symbol(ps, "{");
		left->a = parse_expr(ps, 0);
		expect_symbol(ps, "}");
		left->b = parse_expr(ps, 1);
	} else if (!strcmp(ps->tok->value, "eval")) {
		left->type = EXPR_EVAL;
		NEXT;

		bool paren = !strcmp(ps->tok->value, "(");
		if (paren) expect_symbol(ps, "(");

		left->a = parse_expr(ps, 1);

		if (paren) expect_symbol(ps, ")");
		return left;
	} else if (get_builtin(ps->tok->value)) {
		struct builtin *bi = get_builtin(ps->tok->value);

		left->type = EXPR_BUILTIN;
		left->bi = bi;
		NEXT;

		bool paren = !strcmp(ps->tok->value, "(");
		if (paren) expect_symbol(ps, "(");

		while (true) {
			struct expression *e = parse_expr(ps, bi->prec);

			if (e) {
				left->args = oak_realloc(left->args,
				                         (left->num + 1) * sizeof *left->args);
				left->args[left->num++] = e;
			}

			if (strcmp(ps->tok->value, ",")) break;
			else expect_symbol(ps, ",");
		}

		ps->tok = ps->tok->prev;
		if (!strcmp(ps->tok->value, ","))
			error_push(ps->r, ps->tok->loc, ERR_FATAL, "stray comma");
		NEXT;

		if (paren) expect_symbol(ps, ")");
	} else if (!strcmp(ps->tok->value, "{")) {
		free(left);
		left = parse_table(ps);
	} else { /* if it's not a prefix operator it must be a value. */
		if (ps->tok->type == TOK_INTEGER
			|| ps->tok->type == TOK_STRING
			|| ps->tok->type == TOK_FLOAT
			|| ps->tok->type == TOK_IDENTIFIER
			|| ps->tok->type == TOK_BOOL) {
			left->type = EXPR_VALUE;
			left->val = ps->tok;
			NEXT;
		} else if (ps->tok->type == TOK_REGEX) {
			left->type = EXPR_REGEX;
			left->val = ps->tok;
			NEXT;
		} else if (ps->tok->type == TOK_GROUP) {
			left->type = EXPR_GROUP;
			left->val = ps->tok;
			NEXT;
		} else {
			free(left);
			return NULL;
		}
	}

	op = NULL;
	struct expression *e = left;

	while (prec < get_prec(ps, prec)) {
		op = get_infix_op(ps);
		if (!op) return left;

		e = new_expression(ps->tok);
		e->type = EXPR_OPERATOR;
		e->operator = op;
		e->a = left;

		switch (op->type) {
		case OPTYPE_TERNARY:
			NEXT;
			e->b = parse_expr(ps, 1);
			expect_symbol(ps, op->body2);
			e->c = parse_expr(ps, 1);
			break;

		case OPTYPE_FN_CALL:
			e->type = EXPR_FN_CALL;

			NEXT;
			if (strcmp(ps->tok->value, op->body2)) {
				ps->tok = ps->tok->prev;
				do {
					NEXT;
					e->args = oak_realloc(e->args, (e->num + 1) * sizeof *e->args);
					e->args[e->num++] = parse_expr(ps, 1);
				} while (!strcmp(ps->tok->value, ","));
			}

			expect_symbol(ps, op->body2);
			break;

		case OPTYPE_BINARY:
			NEXT;
			e->b = parse_expr(ps, op->ass == ASS_LEFT ? op->prec : op->prec - 1);
			break;

		case OPTYPE_SUBCRIPT:
			e->type = EXPR_SUBSCRIPT;

			NEXT;
			e->b = parse_expr(ps, 0);
			expect_symbol(ps, op->body2);
			break;

		case OPTYPE_POSTFIX: NEXT; break;
		default:
			DOUT("internal error; an invalid operator was returned.");
			assert(false);
			break;
		}

		left = e;
	}

	return e;
}

static struct expression *
parse_expression(struct parser *ps, int prec)
{
	struct expression *e = parse_expr(ps, prec);

	if (!e) {
		error_push(ps->r, ps->tok->loc, ERR_FATAL, "expected an expression, value, or prefix operator");
		NEXT;
	}

	return e;
}

static struct statement *
parse_if_stmt(struct parser *ps)
{
	struct statement *s = new_statement(ps->tok);
	s->type = STMT_IF_STMT;
	s->if_stmt.otherwise = NULL;

	NEXT;
	s->if_stmt.cond = parse_expression(ps, 0);
	if (!strcmp(ps->tok->value, "{")) {
		s->if_stmt.then = parse_stmt(ps);
	} else {
		expect_symbol(ps, ":");
		s->if_stmt.then = parse_stmt(ps);
	}

	if (ps->tok->type == TOK_KEYWORD
	    && ps->tok->keyword->type == KEYWORD_ELSE) {
		NEXT;
		s->if_stmt.otherwise = parse_stmt(ps);
	}
	return s;
}

static struct statement *
parse_fn_def(struct parser *ps)
{
	NEXT;
	struct statement *s = new_statement(ps->tok);
	s->type = STMT_FN_DEF;

	if (ps->tok->type == TOK_IDENTIFIER) {
		s->fn_def.name = ps->tok->value;
		NEXT;
	} else {
		s->fn_def.name = "";
	}

	if (!strcmp(ps->tok->value, "(")) {
		expect_symbol(ps, "(");

		while (ps->tok->type == TOK_IDENTIFIER) {
			s->fn_def.args = oak_realloc(s->fn_def.args, (s->fn_def.num + 1) * sizeof *s->fn_def.args);
			s->fn_def.args[s->fn_def.num++] = ps->tok;

			s->fn_def.init = oak_realloc(s->fn_def.init, (s->fn_def.num + 1) * sizeof *s->fn_def.init);
			s->fn_def.init[s->fn_def.num - 1] = NULL;

			NEXT;
			if (!strcmp(ps->tok->value, ",")) {
				expect_symbol(ps, ",");
			} else if (!strcmp(ps->tok->value, "=")) {
				NEXT;

				if (!strcmp(ps->tok->value, "...")) {
					struct expression *e = new_expression(ps->tok);
					e->type = EXPR_VARARGS;
					s->fn_def.init[s->fn_def.num - 1] = e;
					NEXT;
				} else {
					s->fn_def.init[s->fn_def.num - 1] = parse_expr(ps, 1);
				}

				if (!strcmp(ps->tok->value, ",")) {
					expect_symbol(ps, ",");
				} else break;
			} else break;
		}

		expect_symbol(ps, ")");
	}

	if (!strcmp(ps->tok->value, "=")) {
		expect_symbol(ps, "=");
		struct statement *body = new_statement(ps->tok);
		body->type = STMT_RET;
		body->ret.expr = parse_expression(ps, 0);
		s->fn_def.body = body;
	} else {
		s->fn_def.body = parse_stmt(ps);
	}

	return s;
}

static struct statement *
parse_print(struct parser *ps)
{
	struct statement *s = new_statement(ps->tok);
	s->type = STMT_PRINT;

	if (ps->tok->next->type == TOK_END) {
		error_push(ps->r, ps->tok->loc, ERR_FATAL,
			   "unexpected end-of-file; expected an expression");
		NEXT;
		return NULL;
	}

	do {
		NEXT;
		s->print.args = oak_realloc(s->print.args,
					    sizeof *(s->print.args) * (s->print.num + 2));

		s->print.args[s->print.num] = parse_expression(ps, 1);
		s->print.num++;
	} while (!strcmp(ps->tok->value, ","));

	s->print.args = oak_realloc(s->print.args,
			    sizeof *(s->print.args) * (s->print.num));
	expect_terminator(ps);

	return s;
}

static struct statement *
parse_println(struct parser *ps)
{
	struct statement *s = parse_print(ps);
	s->type = STMT_PRINTLN;
	return s;
}

static struct statement *
parse_vardecl(struct parser *ps)
{
	struct statement *s = new_statement(ps->tok);
	NEXT;
	s->type = STMT_VAR_DECL;

	while (ps->tok->type == TOK_IDENTIFIER) {
		s->var_decl.names = oak_realloc(s->var_decl.names, sizeof *s->var_decl.names * (s->var_decl.num + 1));
		s->var_decl.names[s->var_decl.num++] = ps->tok;
		NEXT;
	}

	if (!strcmp(ps->tok->value, "=")) {
		s->var_decl.num_init = s->var_decl.num;

		size_t i = 0;
		do {
			NEXT;

			s->var_decl.init = oak_realloc(s->var_decl.init, sizeof *s->var_decl.init * (i + 1));
			s->var_decl.init[i] = parse_expression(ps, 1);

			i++;
		} while (!strcmp(ps->tok->value, ","));

		if (i != s->var_decl.num) {
			error_push(ps->r, s->tok->loc, ERR_FATAL, "the number of initalizers does not match the number of declarations");
		}
	} else {
		s->var_decl.num_init = 0;
	}

	return s;
}

static struct statement *
parse_for_loop(struct parser *ps)
{
	struct statement *s = new_statement(ps->tok);
	s->type = STMT_FOR_LOOP;
	NEXT;

	if (!strcmp(ps->tok->value, "var")) {
		s->for_loop.a = parse_vardecl(ps);
	} else {
		struct statement *e = new_statement(ps->tok);
		e->type = STMT_EXPR;
		e->expr = parse_expression(ps, 0);

		s->for_loop.a = e;
	}

	if (!strcmp(ps->tok->value, ";")) {
		NEXT;
		s->for_loop.b = parse_expression(ps, 0);

		if (!strcmp(ps->tok->value, ";")) {
			NEXT;
			s->for_loop.c = parse_expression(ps, 0);
		}

		if (!strcmp(ps->tok->value, "{")) {
			s->for_loop.body = parse_stmt(ps);
		} else {
			expect_symbol(ps, ":");
			s->for_loop.body = parse_stmt(ps);
		}
	} else {
		/* TODO: error if the first thing is not an expression */
		if (!strcmp(ps->tok->value, "{")) {
			s->for_loop.body = parse_stmt(ps);
		} else {
			expect_symbol(ps, ":");
			s->for_loop.body = parse_stmt(ps);
		}
	}

	return s;
}

static struct statement *
parse_block(struct parser *ps)
{
	struct statement *s	= new_statement(ps->tok);
	s->type			= STMT_BLOCK;
	s->block.stmts		= oak_malloc(sizeof *(s->block.stmts));

	NEXT;

	while (strcmp(ps->tok->value, "}") && ps->tok->type != TOK_END) {
		s->block.stmts = oak_realloc(s->block.stmts, sizeof s->block.stmts[0] * (s->block.num + 1));
		s->block.stmts[s->block.num++] = parse_stmt(ps);
	}

	expect_symbol(ps, "}");

	return s;
}

static struct statement *
parse_ret(struct parser *ps)
{
	struct statement *s = new_statement(ps->tok);
	s->type = STMT_RET;

	NEXT;
	s->expr = parse_expression(ps, 0);
	expect_terminator(ps);

	return s;
}

static struct statement *
parse_while(struct parser *ps)
{
	struct statement *s = new_statement(ps->tok);
	s->type = STMT_WHILE;
	NEXT;

	s->while_loop.cond = parse_expression(ps, 0);
	if (!strcmp(ps->tok->value, ":")) NEXT;

	s->while_loop.body = parse_stmt(ps);

	return s;
}

static struct statement *
parse_do(struct parser *ps)
{
	struct statement *s = new_statement(ps->tok);
	s->type = STMT_DO;
	NEXT;

	s->do_while_loop.body = parse_stmt(ps);
	expect_symbol(ps, "while");
	s->do_while_loop.cond = parse_expression(ps, 0);
	expect_terminator(ps);

	return s;
}

static struct statement *
parse_import(struct parser *ps)
{
	struct statement *s = new_statement(ps->tok);
	s->type = STMT_IMPORT;
	NEXT;
	s->import.name = ps->tok;

	if (ps->tok->type != TOK_STRING) {
		error_push(ps->r, ps->tok->loc, ERR_FATAL, "expected a string");
		parse_expression(ps, 0); /* stop errors from cascading */
	}

	NEXT;

	if (!strcmp(ps->tok->value, "as")) {
		NEXT;
		s->import.as = ps->tok;

		if (ps->tok->type != TOK_IDENTIFIER) {
			error_push(ps->r, ps->tok->loc, ERR_FATAL, "expected a string");
			parse_expression(ps, 0); /* stop errors from cascading */
		}

		NEXT;
	}

	return s;
}

static struct statement *
parse_stmt(struct parser *ps)
{
	struct statement *s = NULL;

	if (!strcmp(ps->tok->value, "{")) {
		s = parse_block(ps);
	} else if (!strcmp(ps->tok->value, ";")) {
		s = new_statement(ps->tok);
		s->type = STMT_NULL;
		NEXT;
	} else if (ps->tok->type == TOK_KEYWORD) {
		switch (ps->tok->keyword->type) {
		case KEYWORD_IF:     s = parse_if_stmt(ps);	break;
		case KEYWORD_FN:     s = parse_fn_def(ps);	break;
		case KEYWORD_PRINT:  s = parse_print(ps);	break;
		case KEYWORD_PRINTLN:s = parse_println(ps);	break;
		case KEYWORD_VAR:
			s = parse_vardecl(ps);
			expect_terminator(ps);
			break;
		case KEYWORD_FOR:    s = parse_for_loop(ps);	break;
		case KEYWORD_RET:    s = parse_ret(ps);		break;
		case KEYWORD_WHILE:  s = parse_while(ps);	break;
		case KEYWORD_DO:     s = parse_do(ps);		break;
		case KEYWORD_IMPORT: s = parse_import(ps);	break;
		case KEYWORD_ELSE:
			error_push(ps->r, ps->tok->loc, ERR_FATAL, "unmatched `else' statement");
			NEXT;
			break;

		case KEYWORD_LAST:
			s = new_statement(ps->tok);
			s->type = STMT_LAST;
			NEXT;
			break;

		case KEYWORD_NEXT:
			s = new_statement(ps->tok);
			s->type = STMT_NEXT;
			NEXT;
			break;

		case KEYWORD_DIE:
			NEXT;
			s = new_statement(ps->tok);
			s->type = STMT_DIE;
			s->expr = parse_expression(ps, 0);
			break;

		case KEYWORD_WHEN:
			error_push(ps->r, ps->tok->loc, ERR_FATAL, "`when' keyword must follow statement");
			NEXT;
			break;

		default:
			DOUT("unimplemented statement parser for %d (%s)", ps->tok->keyword->type, ps->tok->keyword->body);
			NEXT;
			break;
		}
	} else {
		s = new_statement(ps->tok);
		s->type = STMT_EXPR;
		s->expr = parse_expression(ps, 0);
		expect_terminator(ps);
	}

	if (!strcmp(ps->tok->value, "when") && s) {
		NEXT;
		s->condition = parse_expression(ps, 0);
	}

	return s;
}

struct expression *
parse_isolated_expr(struct token *tok)
{
	struct parser *ps = new_parser();
	ps->tok = tok;


	struct expression *expr = parse_expression(ps, 0);
	if (ps->r->fatal) {
		error_write(ps->r, stderr);
		free_parser(ps);

		// TODO: is there a better way to do this?
		exit(EXIT_FAILURE);
	} else if (ps->r->pending) {
		error_write(ps->r, stderr);
	}

	free_parser(ps);

	return expr;
}

bool
parse(struct module *m)
{
	struct parser *ps = new_parser();
	ps->tok = m->tok;

	size_t num = 0;
	struct statement **tree = oak_malloc(sizeof *tree);
	while (ps->tok->type != TOK_END) {
		tree = oak_realloc(tree, sizeof *tree * (num + 2));
		tree[num] = parse_stmt(ps);
		num++;
	}

	tree[num] = NULL;
	m->tree = tree;
	m->num_nodes = num;

	if (ps->r->fatal) {
		error_write(ps->r, stderr);

		for (size_t i = 0; i < num; i++)
			free_stmt(tree[i]);

		free(tree);
		free_parser(ps);
		return false;
	} else if (ps->r->pending) {
		error_write(ps->r, stderr);
	}

	free_parser(ps);
	m->stage = MODULE_STAGE_PARSED;

	return true;
}
