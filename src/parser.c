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
	/* the parser is not responsible for the token stream, so we don't have to free it here. */
	free(ps);
}

static struct Statement *parse_stmt(struct ParseState *ps);
static struct Expression *parse_expr(struct ParseState *ps);

static struct Statement *parse_var_decl(struct ParseState *ps)
{
	token_match(ps->es, &ps->tok, TOK_KEYWORD, "var"); /* hardcoding keywords is a bad idea... */
	
	struct Statement *stmt = oak_malloc(sizeof *stmt);
	stmt->type = STMT_VAR_DECL;
	stmt->var_decl.name = oak_malloc(strlen(ps->tok->value) + 1);
	strcpy(stmt->var_decl.name, ps->tok->value);

	ps->tok = ps->tok->next;
	if (ps->tok->type == TOK_OPERATOR && ps->tok->op_type == OP_EQ_EQ) {
		ps->tok = ps->tok->next;
		stmt->var_decl.init = parse_expr(ps);
	}
	
	return stmt;
}

static struct Expression *parse_expr(struct ParseState *ps)
{
	struct Expression *left = oak_malloc(sizeof *left);

	switch (ps->tok->type) {
	case TOK_INTEGER: {
		left->type = EXPR_INTEGER;
		left->integer = ps->tok->iData;
	} break;
	case TOK_FLOAT: {
		left->type = EXPR_FLOAT;
		left->floating = ps->tok->fData;
	} break;
	case TOK_IDENTIFIER: {
		left->type = EXPR_IDENTIFIER;
		left->identifier = oak_malloc(strlen(ps->tok->value) + 1);
		strcpy(left->identifier, ps->tok->value);
	} break;
	default: printf("TODO: expression: %s\n", ps->tok->value);
	}

	ps->tok = ps->tok->next;
	struct Expression *expr = oak_malloc(sizeof *expr);

	if (ps->tok->type != TOK_OPERATOR) return left;

	switch (ps->tok->op_type) {
	case OP_LESSER_EQ: expr->type = EXPR_OP; /* make it a < node or something */
	default: printf("unsupported operator: %s\n", ps->tok->value);
	}

	ps->tok = ps->tok->next;
	return expr;
}

static struct Expression **parse_expr_list(struct ParseState *ps)
{
	size_t num_exprs = 1;
	struct Expression **list = oak_malloc(sizeof *list);
	do {
		if (ps->tok->type == ',' || ps->tok->type == ')' || ps->tok->type == ';') break;
		list[num_exprs - 1] = parse_expr(ps);
		num_exprs++;
		list = oak_realloc(list, sizeof(*list) * num_exprs);
	} while (true);

	return list;
}

static struct Statement *parse_for_loop(struct ParseState *ps)
{
	error_push(ps->es, ps->tok->loc, ERR_NOTE, "parsing for loop");
	token_match(ps->es, &ps->tok, TOK_KEYWORD, "for");

	struct Statement *stmt = oak_malloc(sizeof *stmt);
	stmt->type = STMT_FOR_LOOP;

	if (ps->tok->type == '(') {
		ps->tok = ps->tok->next;

		/* parse the loop initial statement */
		stmt->for_loop.init = parse_stmt(ps);
		token_expect(ps->es, &ps->tok, TOK_SYMBOL, ";");

		/* parse the condition */
		stmt->for_loop.cond = parse_expr(ps);
		token_expect(ps->es, &ps->tok, TOK_SYMBOL, ";");

		/* parse the things to do on every pass */
		stmt->for_loop.action = parse_expr_list(ps);
		token_expect(ps->es, &ps->tok, TOK_SYMBOL, ")");

		/* parse the body */
		stmt->for_loop.body = parse_stmt(ps);
	} else {
		printf("TODO: parse for-each style loops\n");
	}

	return stmt;
}

static struct Statement *parse_stmt(struct ParseState *ps)
{
	struct Statement *stmt = NULL;

	switch (ps->tok->keyword_type) {
	case KEYWORD_FOR: stmt = parse_for_loop(ps); break;
	case KEYWORD_VAR: stmt = parse_var_decl(ps); break;
	default: error_push(ps->es, ps->tok->loc, ERR_NOTE, "unhandled token");
	}

	return stmt;
}

struct Statement **parse(struct ParseState *ps)
{
	size_t num_stmts = 1;
	struct Statement **program = oak_malloc(sizeof *program * num_stmts);

	while (ps->tok) {
		printf("token: %s\n", ps->tok->value);
		switch (ps->tok->type) {
		case TOK_KEYWORD: program[num_stmts++ - 1] = parse_stmt(ps); break;
		default: 
			ps->tok = ps->tok->next;
		}
	}

	return program;
}
