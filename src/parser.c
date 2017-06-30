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

static void expect_symbol(struct ParseState *ps, char *sym)
{
//	fprintf(stderr, "expecting %s, looking at %s\n", sym, ps->tok->value);
	if (strcmp(ps->tok->value, sym)) {
		if (ps->tok->type == TOK_END) {
			struct Token *tok = ps->tok->prev;

			error_push(ps->es, tok->loc, ERR_FATAL,
				   "unexpected end-of-file; expected '" ERROR_LOCATION_COLOR "%s" RESET_COLOR "'",
				   sym);

			ps->tok = ps->tok->prev;
		} else {
			error_push(ps->es, ps->tok->loc, ERR_FATAL,
				   "unexpected token '"
				   ERROR_LOCATION_COLOR "%s" RESET_COLOR
				   "'; expected '"
				   ERROR_LOCATION_COLOR "%s" RESET_COLOR "'",
				   ps->tok->value, sym);
		}
	}

	ps->tok = ps->tok->next;
}

static void expect_terminator(struct ParseState *ps)
{
//	fprintf(stderr, "starting on token %s\n", ps->tok->value);

	if (strcmp(ps->tok->value, ";") && !ps->tok->prev->is_line_end) {
		if (ps->tok->type == TOK_END) {
			struct Token *tok = ps->tok->prev;

			error_push(ps->es, tok->loc, ERR_FATAL,
				   "unexpected end-of-file; expected a statement terminator");

			ps->tok = ps->tok->prev;
		} else {
			error_push(ps->es, ps->tok->loc, ERR_FATAL,
				   "unexpected token '"
				   ERROR_LOCATION_COLOR "%s" RESET_COLOR
				   "'; expected a statement terminator",
				   ps->tok->value);
		}
	}

	if (!ps->tok->prev->is_line_end)
		ps->tok = ps->tok->next;
//	fprintf(stderr, "ending on token %s\n", ps->tok->value);
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
	struct Expression *left = mkexpr();
	struct Operator *op = get_prefix_op(ps);

	if (op || !strcmp(ps->tok->value, "(")) {
		left->type = EXPR_OPERATOR;
		left->operator = op;

		ps->tok = ps->tok->next;
		if (op) {
			left->a = parse_expr(ps, op->prec);
		} else { /* other wise we're looking at a ( */
			free(left);
			left = parse_expr(ps, 0);
			expect_symbol(ps, ")");
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
			error_push(ps->es, ps->tok->loc, ERR_FATAL, "expected an expression, value or prefix operator");
			ps->tok = ps->tok->next;
		}
	}

	op = NULL;
	struct Expression *e = left;

	while (prec < get_prec(ps, prec)) {
		op = get_infix_op(ps);
		if (!op) return left;

		e = mkexpr();
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
	struct Statement *s = mkstmt(ps->tok);
	s->type = STMT_IF_STMT;
	s->if_stmt.otherwise = NULL;

	ps->tok = ps->tok->next;
	s->if_stmt.cond = parse_expr(ps, 0);
	if (!strcmp(ps->tok->value, "{")) {
		s->if_stmt.then = parse_stmt(ps);
	} else {
		expect_symbol(ps, ":");
		s->if_stmt.then = parse_stmt(ps);
	}

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
	struct Statement *s = mkstmt(ps->tok);
	s->type = STMT_PRINT;
	s->print.num = 0;
	s->print.args = oak_malloc(sizeof *(s->print.args));

	if (ps->tok->next->type == TOK_END) {
		error_push(ps->es, ps->tok->loc, ERR_FATAL,
			   "unexpected end-of-file; expected an expression");
		ps->tok = ps->tok->next;
		return NULL;
	}

	do {
		ps->tok = ps->tok->next;
		s->print.args[s->print.num] = parse_expr(ps, 1);
		s->print.args = oak_realloc(s->print.args,
					    sizeof *(s->print.args) * (s->print.num + 2));
		s->print.num++;
	} while (!strcmp(ps->tok->value, ","));

	s->print.args = oak_realloc(s->print.args,
			    sizeof *(s->print.args) * (s->print.num));
	expect_terminator(ps);

	return s;
}

static struct Statement *parse_stmt(struct ParseState *ps)
{
	struct Statement *s = NULL;

	if (!strcmp(ps->tok->value, "{")) {
		s = parse_block(ps);
	} else if (ps->tok->type == TOK_KEYWORD) {
		switch (ps->tok->keyword->type) {
		case KEYWORD_IF:    s = parse_if_stmt(ps);		break;
		case KEYWORD_FN:    s = parse_fn_def(ps);		break;
		case KEYWORD_PRINT: s = parse_print_stmt(ps);	break;
		default:
			fprintf(stderr, "TODO: parse more statement types.\n");
			ps->tok = ps->tok->next;
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

static struct Statement *parse_block(struct ParseState *ps)
{
	struct Statement *s	= mkstmt(ps->tok);
	s->type		= STMT_BLOCK;
	s->block.stmts		= oak_malloc(sizeof *(s->block.stmts));

	ps->tok = ps->tok->next;

	while (strcmp(ps->tok->value, "}") && ps->tok->type != TOK_END) {
//		fprintf(stderr, "looking at %s\n", ps->tok->value);
		s->block.stmts = oak_realloc(s->block.stmts, sizeof s->block.stmts[0] * (s->block.num + 1));
		s->block.stmts[s->block.num++] = parse_stmt(ps);
	}

	expect_symbol(ps, "}");

	return s;
}

struct Statement **parse(struct ParseState *ps)
{
	size_t num_stmts = 0;
	struct Statement **module = oak_malloc(sizeof *module);

	while (ps->tok->type != TOK_END) {
		module = oak_realloc(module,
				      (num_stmts + 2) * sizeof *module);
		module[num_stmts] = parse_stmt(ps);
		num_stmts++;
	}

	module[num_stmts] = NULL;

	return module;
}
