#include "ast.h"
#include "util.h"

struct StatementData statement_data[] = {
	{ STMT_FN_DEF,		"function definition"	},
	{ STMT_FOR_LOOP,	"for loop"		},
	{ STMT_IF_STMT,	"if"			},
	{ STMT_WHILE_LOOP,	"while loop"		},
	{ STMT_EXPR,		"expression"		},
	{ STMT_VAR_DECL,	"variable declaration"	},
	{ STMT_BLOCK,		"block"		},
	{ STMT_PRINT,		"print"		},
	{ STMT_YIELD,		"yield"		},
	{ STMT_INVALID,	"invalid statement"	}
};

struct Expression *mkexpr(struct Token *tok)
{
	struct Expression *e = oak_malloc(sizeof *e);

	memset(e, 0, sizeof *e);
	e->tok = tok;
	e->type = EXPR_INVALID;

	return e;
}

struct Statement *mkstmt(struct Token *tok)
{
	struct Statement *s = oak_malloc(sizeof *s);

	memset(s, 0, sizeof *s);
	s->tok = tok;
	s->type = STMT_INVALID;

	return s;
}

void free_expr(struct Expression *e)
{
	if (e->type == EXPR_VALUE) {
		/* don't free the tokens here */
	} else if (e->type == EXPR_FN_CALL) {
		free_expr(e->a);
		for (size_t i = 0; i < e->num; i++) {
			free_expr(e->args[i]);
		}
		free(e->args);
	} else {
		if (e->a) free_expr(e->a);
		if (e->b) free_expr(e->b);
		if (e->c) free_expr(e->c);
	}

	free(e);
}

void free_stmt(struct Statement *s)
{
	switch (s->type) {
	case STMT_EXPR:
		free_expr(s->expr);
		break;
	case STMT_BLOCK:
		for (size_t i = 0; i < s->block.num; i++) {
			free_stmt(s->block.stmts[i]);
		}

		free(s->block.stmts);

		break;
	case STMT_IF_STMT:
		free_expr(s->if_stmt.cond);
		free_stmt(s->if_stmt.then);
		if (s->if_stmt.otherwise)
			free_stmt(s->if_stmt.otherwise);
		break;
	case STMT_PRINT:
		for (size_t i = 0; i < s->print.num; i++) {
			free_expr(s->print.args[i]);
		}

		free(s->print.args);

		break;
	case STMT_VAR_DECL:
		for (size_t i = 0; i < s->var_decl.num; i++) {
			free_expr(s->var_decl.init[i]);
		}

		free(s->var_decl.init);
		free(s->var_decl.names);

		break;
	case STMT_FOR_LOOP:
		free_stmt(s->for_loop.a);
		free_expr(s->for_loop.b);
		if (s->for_loop.c) free_expr(s->for_loop.c);
		free_stmt(s->for_loop.body);

		break;
	case STMT_FN_DEF:
		free_stmt(s->fn_def.body);
		free(s->fn_def.args);

		break;
	case STMT_YIELD:
		free_expr(s->yield.expr);
		break;
	default:
		fprintf(stderr, "unimplemented free for statement of type '%d'\n", s->type);
	}

	free(s);
}

void free_ast(struct Statement **module)
{
	struct Statement *s = NULL;

	for (size_t i = 0; (s = module[i]); i++) {
		free_stmt(s);
	}

	free(module);
}
