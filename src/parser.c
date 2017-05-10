#include "parser.h"
#include "error.h"
#include "util.h"

#include <stdbool.h>

static struct Statement *parse_stmt(struct ErrorState *es, struct Token **token);

static struct Statement *parse_var_decl(struct ErrorState *es, struct Token **token)
{
	return NULL;
}

static struct Expression *parse_expr(struct ErrorState *es, struct Token **token)
{
	struct Expression *left = oak_malloc(sizeof *left);
	struct Token *tok = *token;

	switch (tok->type) {
	case TOK_INTEGER: {
		left->type = EXPR_INTEGER;
		left->integer = tok->iData;
	} break;
	case TOK_FLOAT: {
		left->type = EXPR_FLOAT;
		left->floating = tok->fData;
	} break;
	default: printf("TODO: support more things\n");
	}

	struct Expression *expr = oak_malloc(sizeof *expr);

	return expr;
}

static struct Expression *parse_expr_list(struct ErrorState *es, struct Token **token)
{
	
}

static struct Statement *parse_for_loop(struct ErrorState *es, struct Token **token)
{
	struct Token *tok = *token;
	error_push(es, tok->loc, ERR_WARNING, "parsing for loop");
	token_match(es, &tok, TOK_KEYWORD, "for");

	struct Statement *stmt = oak_malloc(sizeof *stmt);
	stmt->type = STMT_FOR_LOOP;

	if (tok->type == TOK_SYMBOL && !strcmp(tok->value, "(")) {
		tok = tok->next;

		/* parse the loop initial statement */
		stmt->for_loop.init = parse_stmt(es, &tok);
		token_expect(es, &tok, TOK_SYMBOL, ";");

		/* parse the condition */
		stmt->for_loop.cond = parse_expr_list(es, &tok);
		token_expect(es, &tok, TOK_SYMBOL, ";");

		/* parse the things to do on every pass */
		stmt->for_loop.action = parse_expr_list(es, &tok);
		token_expect(es, &tok, TOK_SYMBOL, ")");

		/* parse the body */
		stmt->for_loop.body = parse_stmt(es, &tok);
	} else {
		printf("TODO: parse for-each style loops\n");
	}

	return stmt;
}

static struct Statement *parse_stmt(struct ErrorState *es, struct Token **token)
{
	struct Token *tok = *token;
	struct Statement *stmt = NULL;

	switch (tok->keyword_type) {
	case KEYWORD_FOR: stmt = parse_for_loop(es, &tok); break;
	case KEYWORD_VAR: stmt = parse_var_decl(es, &tok); break;
	default: printf("unhandled keyword: %s\n", tok->value);
	}
	
	*token = tok;
	return stmt;
}

struct Statement **parse(struct ErrorState *es, struct Token *tok)
{
	size_t num_stmts = 1;
	struct Statement **program = oak_malloc(sizeof *program * num_stmts);

	while (tok) {
		switch (tok->type) {
		case TOK_KEYWORD: program[num_stmts++ - 1] = parse_stmt(es, &tok); break;
		default: printf("TODO: parse more token types: %s\n", tok->value);
		}
		tok = tok->next;
	}

	return program;
}
