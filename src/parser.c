#include "parser.h"
#include "error.h"
#include "util.h"
#include "operator.h"

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
	if (ps->tok->keyword_type != type) {
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
			   "unexpected token '%s', expected '%s'",
			   ps->tok->value, type_str);
		free(type_str);
	}
}

static void expect_operator(struct ParseState *ps, enum OpType type)
{
	if (ps->tok->type != TOK_OPERATOR || ps->tok->op_type != type) {
		error_push(ps->es, ps->tok->loc, ERR_FATAL,
			   "unexpected token '%s', expected '%s'",
			   ps->tok->value, get_op(type).value);
	}
}

static struct Statement *parse_stmt(struct ParseState *ps);
static struct Statement *parse_block(struct ParseState *ps);

static struct Expression *parse_expr(struct ParseState *ps, int prec)
{
	struct Expression *expr = oak_malloc(sizeof *expr);

	printf("looking at token %s\n", ps->tok->value);

	if (ps->tok->type == TOK_OPERATOR) {
		/* prefix */
		expr->type = EXPR_PREFIX;
	}

	return expr;
}

static struct Statement *parse_if_stmt(struct ParseState *ps)
{
	struct Statement *stmt = oak_malloc(sizeof *stmt);
	stmt->type = STMT_IF_STMT;

	expect_keyword(ps, KEYWORD_IF);
	stmt->if_stmt.cond = parse_expr(ps, 0);
	expect_operator(ps, OP_COLON);
	stmt->if_stmt.body = parse_stmt(ps);

	return stmt;
}

static struct Statement *parse_fn_def(struct ParseState *ps)
{
	/* TODO: parse function definitions */
	return NULL;
}

static struct Statement *parse_stmt(struct ParseState *ps)
{
	struct Statement *ret = oak_malloc(sizeof *ret);

	if (ps->tok->type == (enum TokType)'{') {
		ret = parse_block(ps);
	} else if (ps->tok->type == TOK_KEYWORD) {
		switch (ps->tok->keyword_type) {
		case KEYWORD_IF: ret = parse_if_stmt(ps); break;
		case KEYWORD_FN: ret = parse_fn_def(ps);  break;
		default: printf("TODO: parse more statement types.\n"); break;
		}
	} else {
		ret->type = STMT_EXPR;
		//ret->expr = parse_expr(ps, 0);
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
