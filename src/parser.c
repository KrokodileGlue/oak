#include "parser.h"
#include "error.h"
#include "util.h"
#include "operator.h"
#include "color.h"

#include <stdbool.h>

struct ParseState *parser_new(struct Token *tok)
{
	struct ParseState *ps = oak_malloc(sizeof *ps);

	ps->es = error_new();
	ps->tok = tok;

	return ps;
}

void parser_clear(struct ParseState *ps)
{
	error_clear(ps->es);
	/* the parser is not responsible for the token stream, so we
	 * don't have to free it here. */
	free(ps);
}

static void expect_operator(struct ParseState *ps, char *op_body)
{
	printf("ERROR TOKEN: %s\n", ps->tok->value);
	if (ps->tok->type != TOK_OPERATOR || strcmp(ps->tok->operator->body, op_body)) {
		if (ps->tok->type == TOK_END) {
			struct Token *tok = ps->tok->prev;

			error_push(ps->es, tok->loc, ERR_FATAL,
			   "unexpected end-of-file, expected '" ERROR_LOCATION_COLOR "%s" RESET_COLOR "'",
			   op_body);

			ps->tok = tok;
		} else {
			char *line = get_line(ps->tok->loc);
			error_push(ps->es, ps->tok->loc, ERR_FATAL,
			   "unexpected token '"ERROR_LOCATION_COLOR"%.*s"RESET_COLOR"', expected '" ERROR_LOCATION_COLOR "%s" RESET_COLOR "'",
			   ps->tok->loc.len, line + index_in_line(ps->tok->loc), op_body);
		}
	}
	ps->tok = ps->tok->next;
}

static void expect_symbol(struct ParseState *ps, char *sym)
{
	if (strcmp(ps->tok->value, sym)) {
		error_push(ps->es, ps->tok->loc, ERR_FATAL,
			   "unexpected token '"
			   ERROR_LOCATION_COLOR "%s" RESET_COLOR
			   "', expected '"
			   ERROR_LOCATION_COLOR "%s" RESET_COLOR "'",
			   ps->tok->value, sym);
	}

	ps->tok = ps->tok->next;
}

static struct Statement *parse_stmt(struct ParseState *ps);
static struct Statement *parse_block(struct ParseState *ps);

static struct Operator *get_infix_op(struct ParseState *ps)
{
	for (size_t i = 0; i < num_ops(); i++) {
		if (!strcmp(ops[i].body, ps->tok->value)
		    && ops[i].type != OP_PREFIX) {
			return ops + i;
		}
	}

	return NULL;
}

static struct Operator *get_prefix_op(struct ParseState *ps)
{
	for (size_t i = 0; i < num_ops(); i++) {
		if (!strcmp(ops[i].body, ps->tok->value)
		    && ops[i].type == OP_PREFIX) {
			return ops + i;
		}
	}

	return NULL;
}

static size_t get_prec(struct ParseState *ps, size_t prec)
{
	if (get_infix_op(ps))
		return get_infix_op(ps)->prec;

	return prec;
}

static struct Expression *parse_expr(struct ParseState *ps, size_t prec)
{
	struct Expression *left = oak_malloc(sizeof *left);
	struct Operator *op = ps->tok->operator;

	if (ps->tok->type == TOK_OPERATOR
	    && (op->type == OP_PREFIX || !strcmp(op->body, "("))) {
		left->type = EXPR_OPERATOR;
		left->operator = op;

		ps->tok = ps->tok->next;
		if (!strcmp(op->body, "(")) {
			free(left);
			left = parse_expr(ps, 0);
			expect_symbol(ps, ")");
		} else {
			left->a = parse_expr(ps, op->prec);
		}
	} else { /* if it's not a prefix operator it must be a value. */
		if (ps->tok->type == TOK_INTEGER
			|| ps->tok->type == TOK_STRING
			|| ps->tok->type == TOK_FLOAT
			|| ps->tok->type == TOK_IDENTIFIER
			|| ps->tok->type == TOK_BOOL) {
			left->type = EXPR_VALUE;
			left->value = ps->tok;
			ps->tok = ps->tok->next;
		} else {
			error_push(ps->es, ps->tok->loc, ERR_FATAL, "expected an expression value or prefix operator");
			ps->tok = ps->tok->next;
		}
	}

	op = NULL;
	struct Expression *e = left;

	while (prec < get_prec(ps, prec)) {
		op = ps->tok->operator;
		if (!op) return left;

		e = oak_malloc(sizeof *e);
		e->type = EXPR_OPERATOR;
		e->operator = op;
		e->a = left;

		if (op->type == OP_TERNARY) {
			ps->tok = ps->tok->next;
			e->b = parse_expr(ps, 1);
			expect_symbol(ps, op->body2);
			e->c = parse_expr(ps, 1);
		} else if (op->type == OP_MEMBER) {
			ps->tok = ps->tok->next;
			e->b = parse_expr(ps, 0);
			ps->tok = ps->tok->next;
		} else if (op->type == OP_BINARY) {
			ps->tok = ps->tok->next;
			e->b = parse_expr(ps, op->ass == ASS_LEFT ? op->prec : op->prec - 1);
		} else if (op->type == OP_POSTFIX) {
			ps->tok = ps->tok->next;
		}

		left = e;
	}

	return e;
}

static struct Statement *parse_if_stmt(struct ParseState *ps)
{
	struct Statement *s = oak_malloc(sizeof *s);
	s->type = STMT_IF_STMT;
	s->if_stmt.otherwise = NULL;

	ps->tok = ps->tok->next;
	s->if_stmt.cond = parse_expr(ps, 0);
	expect_symbol(ps, ":");
	s->if_stmt.then = parse_stmt(ps);

	if (ps->tok->type == TOK_KEYWORD
	    && ps->tok->keyword->type == KEYWORD_ELSE) {
		ps->tok = ps->tok->next;
		s->if_stmt.otherwise = parse_stmt(ps);
	}
	return s;
}

static struct Statement *parse_fn_def(struct ParseState *ps)
{
	printf("TODO: parse function definitions\n");
	return NULL;
}

static struct Statement *parse_print_stmt(struct ParseState *ps)
{
	struct Statement *s = oak_malloc(sizeof *s);
	s->type = STMT_PRINT;
	s->print.num = 0;
	s->print.args = oak_malloc(sizeof *(s->print.args));

	do {
		ps->tok = ps->tok->next;
		s->print.args[s->print.num] = parse_expr(ps, 1);
		s->print.args = oak_realloc(s->print.args,
					    sizeof *(s->print.args) * (s->print.num + 1));
		s->print.num++;
	} while (ps->tok->type == TOK_OPERATOR
		 && !strcmp(ps->tok->operator->body, ","));

	expect_symbol(ps, ";");

	return s;
}

static struct Statement *parse_stmt(struct ParseState *ps)
{
	struct Statement *ret = oak_malloc(sizeof *ret);

	if (ps->tok->type == (enum TokType)'{') {
		ret			= parse_block(ps);
	} else if (ps->tok->type == TOK_KEYWORD) {
		switch (ps->tok->keyword->type) {
		case KEYWORD_IF:    ret = parse_if_stmt(ps);		break;
		case KEYWORD_FN:    ret = parse_fn_def(ps);		break;
		case KEYWORD_PRINT: ret = parse_print_stmt(ps);	break;
		default:
			printf("TODO: parse more statement types.\n");
			ps->tok = ps->tok->next;
			break;
		}
	} else {
		ret->type = STMT_EXPR;
		ret->expr = parse_expr(ps, 0);
		expect_symbol(ps, ";");
	}

	return ret;
}

static struct Statement *parse_block(struct ParseState *ps)
{
	struct Statement *stmt = oak_malloc(sizeof *stmt);
	stmt->type = STMT_BLOCK;
	stmt->block.stmts = oak_malloc(sizeof *(stmt->block.stmts));

	while (ps->tok->type != (enum TokType)'}') {
		stmt->block.stmts =
			oak_realloc(stmt->block.stmts,
				    sizeof *(stmt->block.stmts)
				    * (stmt->block.num + 1));
		stmt->block.stmts[stmt->block.num++] = parse_stmt(ps);
	}

	/* TODO: check for the token being NULL at this point. */

	return stmt;
}

struct Statement **parse(struct ParseState *ps)
{
	size_t num_stmts = 0;
	struct Statement **program = oak_malloc(sizeof *program);

	while (ps->tok->type != TOK_END) {
		program = oak_realloc(program,
				      (num_stmts + 1) * sizeof *program);
		program[num_stmts] = parse_stmt(ps);
		num_stmts++;
	}

	program[num_stmts] = NULL;

	return program;
}
