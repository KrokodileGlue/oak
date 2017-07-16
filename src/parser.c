#include "parser.h"
#include "error.h"
#include "util.h"
#include "operator.h"
#include "color.h"
#include "module.h"

#include <stdbool.h>
#include <assert.h>

#define NEXT if (ps->tok->type != TOK_END) ps->tok = ps->tok->next;

struct parser *
parser_new(struct token *tok)
{
	struct parser *ps = oak_malloc(sizeof *ps);

	ps->es = error_new();
	ps->tok = tok;

	return ps;
}

void
parser_clear(struct parser *ps)
{
	error_clear(ps->es);
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

			error_push(ps->es, tok->loc, ERR_FATAL,
				   "unexpected end-of-file; expected '" ERROR_LOCATION_COLOR "%s" RESET_COLOR "'",
				   sym);
		} else {
			error_push(ps->es, ps->tok->loc, ERR_FATAL,
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
	if (strcmp(ps->tok->value, ";") && !ps->tok->prev->is_line_end) {
		if (ps->tok->type == TOK_END) {
			struct token *tok = ps->tok->prev;

			error_push(ps->es, tok->loc, ERR_FATAL,
				   "unexpected end-of-file; expected a statement terminator");
		} else {
			error_push(ps->es, ps->tok->loc, ERR_FATAL,
				   "unexpected token '"
				   ERROR_LOCATION_COLOR "%s" RESET_COLOR
				   "'; expected a statement terminator",
				   ps->tok->value);
		}
	}

	if (!strcmp(ps->tok->value, ";")) NEXT;
}

static struct statement *
parse_stmt(struct parser *ps);

static struct operator *
get_infix_op(struct parser *ps)
{
	for (size_t i = 0; i < num_ops(); i++) {
		if (!strcmp(ops[i].body, ps->tok->value)
		    && ops[i].type != OP_PREFIX) {
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
		    && ops[i].type == OP_PREFIX) {
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
	struct expression *left = mkexpr(ps->tok);
	struct operator *op = get_prefix_op(ps);

	if (op || !strcmp(ps->tok->value, "(")) {
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
				/* we have a list comprehension */
				left->type = EXPR_LIST_COMPREHENSION;
				left->a = first;
				NEXT;
				left->b = parse_expr(ps, 0);
				expect_symbol(ps, "in");
				left->c = parse_expr(ps, 0);
			} else {
				NEXT;
				left->args = oak_realloc(left->args, sizeof left->args[0] * (left->num + 1));
				left->args[left->num++] = first;
				do {
					left->args = oak_realloc(left->args, sizeof left->args[0] * (left->num + 1));
					left->args[left->num++] = parse_expr(ps, 1);
					if (!strcmp(ps->tok->value, ",")) NEXT;
				} while (strcmp(ps->tok->value, "]"));
			}

			expect_symbol(ps, "]");
		}
	} else { /* if it's not a prefix operator it must be a value. */
		if (ps->tok->type == TOK_INTEGER
			|| ps->tok->type == TOK_STRING
			|| ps->tok->type == TOK_FLOAT
			|| ps->tok->type == TOK_IDENTIFIER
			|| ps->tok->type == TOK_BOOL) {
			left->type = EXPR_VALUE;
			left->val = ps->tok;
			NEXT;
		} else {
			error_push(ps->es, ps->tok->loc, ERR_FATAL, "expected an expression, value or prefix operator");
			NEXT;
		}
	}

	op = NULL;
	struct expression *e = left;

	while (prec < get_prec(ps, prec)) {
		op = get_infix_op(ps);
		if (!op) return left;

		e = mkexpr(ps->tok);
		e->type = EXPR_OPERATOR;
		e->operator = op;
		e->a = left;

		switch (op->type) {
		case OP_TERNARY:
			NEXT;
			e->b = parse_expr(ps, 1);
			expect_symbol(ps, op->body2);
			e->c = parse_expr(ps, 1);
			break;
		case OP_FN_CALL:
			e->type = EXPR_FN_CALL;

			NEXT;
			if (strcmp(ps->tok->value, op->body2)) {
				ps->tok = ps->tok->prev;
				do {
					NEXT;
					e->args = oak_realloc(e->args, sizeof e->args[0] * (e->num + 1));
					e->args[e->num++] = parse_expr(ps, 1);
				} while (!strcmp(ps->tok->value, ","));
			}

			expect_symbol(ps, op->body2);
			break;
		case OP_BINARY:
			NEXT;
			e->b = parse_expr(ps, op->ass == ASS_LEFT ? op->prec : op->prec - 1);
			break;
		case OP_PREFIX: NEXT; break;
		case OP_SUBCRIPT:
			e->type = EXPR_SUBSCRIPT;

			NEXT;
			e->b = parse_expr(ps, 0);
			expect_symbol(ps, op->body2);
			break;
		case OP_POSTFIX: NEXT; break;
		default:
			DOUT("\noak: internal error; an invalid operator was returned.");
			assert(false);
			break;
		}

		left = e;
	}

	return e;
}

static struct statement *
parse_if_stmt(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_IF_STMT;
	s->if_stmt.otherwise = NULL;

	NEXT;
	s->if_stmt.cond = parse_expr(ps, 0);
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
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_FN_DEF;

	if (ps->tok->type != TOK_IDENTIFIER) {
		error_push(ps->es, ps->tok->loc, ERR_FATAL, "expected an identifier");
	}

	if (ps->tok->type != TOK_IDENTIFIER)
		error_push(ps->es, ps->tok->loc, ERR_FATAL, "expected an identifier");
	s->fn_def.name = ps->tok->value;
	NEXT;

	while (ps->tok->type == TOK_IDENTIFIER) {
		s->fn_def.args = oak_realloc(s->fn_def.args, sizeof *s->fn_def.args * (s->fn_def.num + 1));
		s->fn_def.args[s->fn_def.num++] = ps->tok;
		NEXT;
	}

	if (!strcmp(ps->tok->value, "=")) {
		expect_symbol(ps, "=");
		s->fn_def.body = parse_stmt(ps);
	} else {
		s->fn_def.body = parse_stmt(ps);
	}

	return s;
}

static struct statement *
parse_print_stmt(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_PRINT;

	if (ps->tok->next->type == TOK_END) {
		error_push(ps->es, ps->tok->loc, ERR_FATAL,
			   "unexpected end-of-file; expected an expression");
		NEXT;
		return NULL;
	}

	do {
		NEXT;
		s->print.args = oak_realloc(s->print.args,
					    sizeof *(s->print.args) * (s->print.num + 2));

		s->print.args[s->print.num] = parse_expr(ps, 1);
		s->print.num++;
	} while (!strcmp(ps->tok->value, ","));

	s->print.args = oak_realloc(s->print.args,
			    sizeof *(s->print.args) * (s->print.num));
	expect_terminator(ps);

	return s;
}

static struct statement *
parse_vardecl(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
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
			s->var_decl.init[i] = parse_expr(ps, 1);

			i++;
		} while (!strcmp(ps->tok->value, ","));

		if (i != s->var_decl.num) {
			error_push(ps->es, s->tok->loc, ERR_FATAL, "the number of initalizers does not match the number of declarations");
		}
	} else {
		s->var_decl.num_init = 0;
	}

	return s;
}

static struct statement *
parse_for_loop(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_FOR_LOOP;
	NEXT;

	if (!strcmp(ps->tok->value, "var")) {
		s->for_loop.a = parse_vardecl(ps);
	} else {
		struct statement *e = mkstmt(ps->tok);
		e->type = STMT_EXPR;
		e->expr = parse_expr(ps, 0);

		s->for_loop.a = e;
	}

	if (!strcmp(ps->tok->value, "in")) {
		NEXT;
		s->for_loop.b = parse_expr(ps, 0);

		if (!strcmp(ps->tok->value, "{")) {
			s->for_loop.body = parse_stmt(ps);
		} else {
			expect_symbol(ps, ":");
			s->for_loop.body = parse_stmt(ps);
		}
	} else {
		expect_symbol(ps, ";");

		s->for_loop.b = parse_expr(ps, 0);
		expect_symbol(ps, ";");

		s->for_loop.c = parse_expr(ps, 0);

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
	struct statement *s	= mkstmt(ps->tok);
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
parse_yield(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_YIELD;

	NEXT;
	s->expr = parse_expr(ps, 0);
	expect_terminator(ps);

	return s;
}

static struct statement *
parse_while(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_WHILE;
	NEXT;

	s->while_loop.cond = parse_expr(ps, 0);
	if (!strcmp(ps->tok->value, ":")) NEXT;

	s->while_loop.body = parse_stmt(ps);

	return s;
}

static struct statement *
parse_do(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_DO;
	NEXT;

	s->do_while_loop.body = parse_stmt(ps);
	expect_symbol(ps, "while");
	s->do_while_loop.cond = parse_expr(ps, 0);
	expect_terminator(ps);

	return s;
}

static struct statement *
parse_class(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_CLASS;

	NEXT;
	if (ps->tok->type != TOK_IDENTIFIER)
		error_push(ps->es, ps->tok->loc, ERR_FATAL, "expected an identifier");
	s->class.name = ps->tok->value;
	NEXT;

	if (!strcmp(ps->tok->value, ":")) {
		NEXT;
		s->class.parent_name = ps->tok;
		NEXT;
	}

	expect_symbol(ps, "{");

	while (strcmp(ps->tok->value, "}")) {
		struct token *tok = ps->tok; /* so that the error is reported at the right place */
		s->class.body = oak_realloc(s->class.body, sizeof s->class.body[0] * (s->class.num + 1));
		s->class.body[s->class.num] = parse_stmt(ps);

		if (s->class.body[s->class.num]->type == STMT_FN_DEF
		    || s->class.body[s->class.num]->type == STMT_VAR_DECL) {
			// good.
		} else {
			error_push(ps->es, tok->loc, ERR_FATAL, "class bodies may not contain arbitrary code");
		}

		s->class.num++;
	}

	expect_symbol(ps, "}");

	return s;
}

static struct statement *
parse_import(struct parser *ps)
{
	struct statement *s = mkstmt(ps->tok);
	s->type = STMT_IMPORT;
	NEXT;
	s->import.name = ps->tok;

	if (ps->tok->type != TOK_IDENTIFIER) {
		error_push(ps->es, ps->tok->loc, ERR_FATAL, "expected an identifier");
		parse_expr(ps, 0); /* stop errors from cascading */
	}

	NEXT;

	if (!strcmp(ps->tok->value, "as")) {
		NEXT;
		s->import.as = ps->tok;
		NEXT;
	}

	expect_symbol(ps, ";");

	return s;
}

static struct statement *
parse_stmt(struct parser *ps)
{
	struct statement *s = NULL;

	if (!strcmp(ps->tok->value, "{")) {
		s = parse_block(ps);
	} else if (ps->tok->type == TOK_KEYWORD) {
		switch (ps->tok->keyword->type) {
		case KEYWORD_IF:     s = parse_if_stmt(ps);	break;
		case KEYWORD_FN:     s = parse_fn_def(ps);	break;
		case KEYWORD_PRINT:  s = parse_print_stmt(ps);	break;
		case KEYWORD_VAR:
			s = parse_vardecl(ps);
			expect_terminator(ps);
			break;
		case KEYWORD_FOR:    s = parse_for_loop(ps);	break;
		case KEYWORD_YIELD:  s = parse_yield(ps);	break;
		case KEYWORD_WHILE:  s = parse_while(ps);	break;
		case KEYWORD_DO:     s = parse_do(ps);		break;
		case KEYWORD_CLASS:  s = parse_class(ps);	break;
		case KEYWORD_IMPORT: s = parse_import(ps);	break;

		default:
			DOUT("\nunimplemented statement parser for %d (%s)\n", ps->tok->keyword->type, ps->tok->keyword->body);
			NEXT;
			break;
		}
	} else {
		s = mkstmt(ps->tok);
		s->type = STMT_EXPR;
		s->expr = parse_expr(ps, 0);
		expect_terminator(ps);
	}

	return s;
}

struct module *
parse(struct parser *ps)
{
	size_t num = 0;
	struct statement **tree = oak_malloc(sizeof *tree);
	while (ps->tok->type != TOK_END) {
		tree = oak_realloc(tree, sizeof *tree * (num + 2));
		tree[num] = parse_stmt(ps);
		num++;
	}
	tree[num] = NULL;

	struct module *module = new_module(ps->tok->loc.file, tree, num, ps->tok);

	return module;
}
