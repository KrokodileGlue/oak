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

static void expect_keyword(struct ParseState *ps, enum KeywordType type)
{
	if (ps->tok->keyword != type) {
		error_push(ps->es, ps->tok->loc, ERR_FATAL,
			   "unexpected token '%s', expected '%s'.",
			   ps->tok->value, keyword_get_type_str(type));
	}
	ps->tok = ps->tok->next;
}

static void expect_toktype(struct ParseState *ps, enum TokType type)
{
	if (ps->tok->type != type) {
		char *type_str = token_get_str(type);
		error_push(ps->es, ps->tok->loc, ERR_FATAL,
			   "unexpected token '"ERROR_HIGHLIGHT_COLOR"%s"RESET_COLOR"', expected '%s'",
			   ps->tok->value, type_str);
		free(type_str);
	}
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

static struct Expression *parse_expr(struct ParseState *ps, size_t prec)
{
	fprintf(stderr, "token: %s\n", ps->tok->value);

	struct Expression *left = oak_malloc(sizeof *left);
	struct Operator *op = ps->tok->operator;

	if (ps->tok->type == TOK_OPERATOR) {
		left->type = EXPR_OPERATOR;
		left->operator = op;

		ps->tok = ps->tok->next;
		if (op->type == OP_GROUP) {
			left->a = parse_expr(ps, 0);
			expect_symbol(ps, op->body2);
			ps->tok = ps->tok->next;
		} else {
			left->a = parse_expr(ps, op->prec);
		}
	} else {
		left->type = EXPR_VALUE;
		left->value = ps->tok;
		ps->tok = ps->tok->next;
	}

	op = NULL;
	struct Expression *e = left;

	while (prec <
	       (ps->tok->type == TOK_OPERATOR
	       ? ps->tok->operator->prec
	       : prec)) {
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
			ps->tok = ps->tok->next;
		} else if (op->type == OP_MEMBER) {
			ps->tok = ps->tok->next;
			e->b = parse_expr(ps, 0);
			expect_symbol(ps, op->body2);
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
	struct Statement *stmt = oak_malloc(sizeof *stmt);
	stmt->type = STMT_IF_STMT;

	expect_keyword(ps, KEYWORD_IF);
	stmt->if_stmt.cond = parse_expr(ps, 0);
	expect_symbol(ps, ":");
	stmt->if_stmt.body = parse_stmt(ps);

	return stmt;
}

static struct Statement *parse_fn_def(struct ParseState *ps)
{
	/* TODO: parse function definitions */
	return NULL;
}

static struct Statement *parse_print_stmt(struct ParseState *ps)
{
	fprintf(stderr, "parse if statements n shit\n");
	ps->tok = ps->tok->next;
	return NULL;
}

static struct Statement *parse_stmt(struct ParseState *ps)
{
	struct Statement *ret = oak_malloc(sizeof *ret);

	if (ps->tok->type == (enum TokType)'{') {
		ret = parse_block(ps);
	} else if (ps->tok->type == TOK_KEYWORD) {
		switch (ps->tok->keyword) {
		case KEYWORD_IF:    ret = parse_if_stmt(ps); break;
		case KEYWORD_FN:    ret = parse_fn_def(ps);  break;
		case KEYWORD_PRINT: ret = parse_print_stmt(ps);  break;
		default: printf("TODO: parse more statement types.\n"); break;
		}
	} else {
		ret->type = STMT_EXPR;
		ret->expr = parse_expr(ps, 0);
		expect_operator(ps, ";");
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
		fprintf(stderr, "parsing a statment with value '%s'\n",
			ps->tok->value);
		program = oak_realloc(program,
				      (num_stmts + 1) * sizeof *program);
		program[num_stmts] = parse_stmt(ps);
	}

	return program;
}
