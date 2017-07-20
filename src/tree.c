#include "keyword.h"
#include "util.h"
#include "token.h"
#include "assert.h"
#include "tree.h"
#include "util.h"

struct statementData statement_data[] = {
	{ STMT_FN_DEF,		"function definition"	},
	{ STMT_FOR_LOOP,	"for loop"		},
	{ STMT_IF_STMT,		"if"			},
	{ STMT_WHILE,		"while loop"		},
	{ STMT_DO,		"do-while loop"		},
	{ STMT_EXPR,		"expression"		},
	{ STMT_VAR_DECL,	"variable declaration"	},
	{ STMT_BLOCK,		"block"			},
	{ STMT_PRINT,		"print"			},
	{ STMT_YIELD,		"yield"			},
	{ STMT_CLASS,		"class"			},
	{ STMT_IMPORT,		"import"			},
	{ STMT_INVALID,		"invalid statement"	}
};

struct expression *
new_expression(struct token *tok)
{
	struct expression *e = oak_malloc(sizeof *e);

	memset(e, 0, sizeof *e);
	e->tok = tok;
	e->type = EXPR_INVALID;

	return e;
}

struct statement *
new_statement(struct token *tok)
{
	struct statement *s = oak_malloc(sizeof *s);

	memset(s, 0, sizeof *s);
	s->tok = tok;
	s->type = STMT_INVALID;

	return s;
}

void
free_expr(struct expression *e)
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

void
free_stmt(struct statement *s)
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
		for (size_t i = 0; i < s->var_decl.num_init; i++) {
			free_expr(s->var_decl.init[i]);
		}

		free(s->var_decl.init);
		free(s->var_decl.names);

		break;
	case STMT_FOR_LOOP:
		free_stmt(s->for_loop.a);
		free_expr(s->for_loop.b);
		if (s->for_loop.c)
			free_expr(s->for_loop.c);
		free_stmt(s->for_loop.body);

		break;
	case STMT_FN_DEF:
		free_stmt(s->fn_def.body);
		free(s->fn_def.args);

		break;
	case STMT_YIELD:
		free_expr(s->yield.expr);

		break;
	case STMT_WHILE:
		free_expr(s->while_loop.cond);
		free_stmt(s->while_loop.body);

		break;
	case STMT_DO:
		free_expr(s->do_while_loop.cond);
		free_stmt(s->do_while_loop.body);

		break;
	case STMT_CLASS:
		for (size_t i = 0; i < s->class.num; i++) {
			free_stmt(s->class.body[i]);
		}
		free(s->class.body);

		break;
	case STMT_IMPORT:
		break;
	default:
		fprintf(stderr, "unimplemented free for statement of type %d (%s)\n",
		        s->type, statement_data[s->type].body);
	}

	free(s);
}

void
free_ast(struct statement **module)
{
	struct statement *s = NULL;

	for (size_t i = 0; (s = module[i]); i++) {
		free_stmt(s);
	}

	free(module);
}

struct ASTPrinter {
	FILE *f;
	size_t depth;
	int arm[2048]; /* this is probably enough. */
};

static void
split(struct ASTPrinter *ap)
{
	ap->arm[ap->depth - 1] = 1;
}

static void
join(struct ASTPrinter *ap)
{
	ap->arm[ap->depth - 1] = 0;
}

static void
indent(struct ASTPrinter *ap)
{
	fprintf(ap->f, "\n");

	for (size_t i = 0; i < (ap->depth ? ap->depth - 1 : 0); i++) {
		if (ap->arm[i])
			fprintf(ap->f, "|   ");
		else
			fprintf(ap->f, "    ");
	}

	if (ap->depth) {
		if (ap->arm[ap->depth - 1])
			fprintf(ap->f, "|-- ");
		else
			fprintf(ap->f, "`-- ");
	}
}

static void
print_expression(struct ASTPrinter *ap, struct expression *e)
{
	indent(ap);

	if (e->type == EXPR_OPERATOR || e->type == EXPR_FN_CALL || e->type == EXPR_SUBSCRIPT) {
		struct operator *op = e->operator;

		switch (op->type) {
		case OP_PREFIX:
			fprintf(ap->f,"(prefix %s)", op->body);
			ap->depth++;
			print_expression(ap, e->a);
			ap->depth--;
			break;
		case OP_BINARY:
			fprintf(ap->f,"(binary %s)", op->body);

			ap->depth++;

			split(ap);
			print_expression(ap, e->a);
			join(ap);
			print_expression(ap, e->b);

			ap->depth--;
			break;
		case OP_TERNARY:
			fprintf(ap->f,"(ternary %s)", op->body);

			ap->depth++;

			split(ap);
			print_expression(ap, e->a);
			print_expression(ap, e->b);
			join(ap);
			print_expression(ap, e->c);

			ap->depth--;
			break;
		case OP_POSTFIX:
			fprintf(ap->f,"(postfix %s)", op->body);
			ap->depth++;
			print_expression(ap, e->a);
			ap->depth--;
			break;
		case OP_FN_CALL:
			fprintf(ap->f,"(fn call)");
			ap->depth++; split(ap);

			print_expression(ap, e->a);
			join(ap); indent(ap);
			fprintf(ap->f, "(arguments)");

			ap->depth++;
			split(ap);
			for (size_t i = 0; i < e->num; i++) {
				if (i == e->num - 1) join(ap);
				print_expression(ap, e->args[i]);
			}
			join(ap);
			ap->depth -= 2;
			break;
		case OP_SUBCRIPT:
			fprintf(ap->f,"(subscript)");
			ap->depth++;
			split(ap);
			print_expression(ap, e->b);
			join(ap);
			indent(ap);
			fprintf(ap->f, "(of)");
			ap->depth++;
			print_expression(ap, e->a);
			ap->depth -= 2;
			break;
		case OP_INVALID:
			fprintf(stderr, "\noak: internal error; an invalid operator node was encountered.\n");
			assert(false);
			break;
		}
	} else {
		switch (e->val->type) {
		case TOK_IDENTIFIER:
			fprintf(ap->f, "(identifier %s)", e->val->value);
			break;
		case TOK_INTEGER:
			fprintf(ap->f, "(integer %zd)", e->val->integer);
			break;
		case TOK_STRING:
			fprintf(ap->f, "(string \"%s\")", e->val->string);
			break;
		case TOK_FLOAT:
			fprintf(ap->f, "(float %f)", e->val->floating);
			break;
		case TOK_BOOL:
			fprintf(ap->f, "(bool %s)", e->val->boolean ? "true" : "false");
			break;
		default:
			fprintf(stderr, "impossible value token %d\n", e->type);
			assert(false);
		}
	}
}

static void
print_statement(struct ASTPrinter *ap, struct statement *s)
{
	indent(ap);
	fprintf(ap->f, "(stmt %s)", statement_data[s->type].body);

	switch (s->type) {
	case STMT_IF_STMT:
		ap->depth++;
		split(ap);

		indent(ap);  fprintf(ap->f, "(condition)");
		ap->depth++; print_expression(ap, s->if_stmt.cond); ap->depth--;

		if (s->if_stmt.otherwise) {
			indent(ap);  fprintf(ap->f, "(then branch)");
			ap->depth++; print_statement(ap, s->if_stmt.then); ap->depth--;

			join(ap);
			indent(ap);  fprintf(ap->f, "(else branch)");
			ap->depth++; print_statement(ap, s->if_stmt.otherwise); ap->depth--;
		} else {
			join(ap);
			indent(ap);  fprintf(ap->f, "(then branch)");
			ap->depth++; print_statement(ap, s->if_stmt.then); ap->depth--; join(ap);
		}

		ap->depth--;
		break;
	case STMT_PRINT:
		ap->depth++;
		split(ap);

		for (size_t i = 0; i < s->print.num; i++) {
			if (i == s->print.num - 1) join(ap);
			print_expression(ap, s->print.args[i]);
		}

		ap->depth--;
		break;
	case STMT_EXPR:
		ap->depth++;
		print_expression(ap, s->expr);
		ap->depth--;
		break;
	case STMT_BLOCK:
		ap->depth++;
		split(ap);

		for (size_t i = 0; i < s->block.num; i++) {
			if (i == s->print.num - 1) join(ap);
			print_statement(ap, s->block.stmts[i]);
		}

		ap->depth--;
		break;
	case STMT_VAR_DECL:
		ap->depth++;
		split(ap);

		for (size_t i = 0; i < s->var_decl.num; i++) {
			if (i == s->var_decl.num - 1) join(ap);
			indent(ap);
			fprintf(ap->f, "(%s)", s->var_decl.names[i]->value);
			if (s->var_decl.num_init) {
				ap->depth++;
				print_expression(ap, s->var_decl.init[i]);
				ap->depth--;
			}
		}

		ap->depth--;
		break;
	case STMT_FOR_LOOP:
		ap->depth++;
		split(ap);

		indent(ap);  fprintf(ap->f, "(initializer)");
		ap->depth++; print_statement(ap, s->for_loop.a); ap->depth--;

		indent(ap);
		if (s->for_loop.c) fprintf(ap->f, "(condition)");
		else fprintf(ap->f, "(iterator)");
		ap->depth++; print_expression(ap, s->for_loop.b); ap->depth--;

		if (s->for_loop.c) {
			indent(ap);  fprintf(ap->f, "(next)");
			ap->depth++; print_expression(ap, s->for_loop.c); ap->depth--;
		}

		join(ap);
		indent(ap);  fprintf(ap->f, "(body)");
		ap->depth++; print_statement(ap, s->for_loop.body); ap->depth--;

		ap->depth--;
		break;
	case STMT_FN_DEF:
		ap->depth++;
		split(ap);

		indent(ap);  fprintf(ap->f, "(name)");
		ap->depth++; indent(ap); fprintf(ap->f, "(%s)", s->fn_def.name); ap->depth--;

		indent(ap);  fprintf(ap->f, "(arguments)");

		ap->depth++;
		split(ap);
		for (size_t i = 0; i < s->fn_def.num; i++) {
			if (i == s->fn_def.num - 1) join(ap);
			indent(ap);
			fprintf(ap->f, "(%s)", s->fn_def.args[i]->value);
		}
		join(ap);
		ap->depth--;

		join(ap);
		indent(ap);  fprintf(ap->f, "(body)");

		ap->depth++;
		print_statement(ap, s->for_loop.body);
		ap->depth--;

		ap->depth--;
		break;
	case STMT_YIELD:
		ap->depth++;
		print_expression(ap, s->expr);
		ap->depth--;
		break;
	case STMT_WHILE:
		ap->depth++;
		split(ap);

		indent(ap);  fprintf(ap->f, "(condition)");
		ap->depth++; print_expression(ap, s->while_loop.cond); ap->depth--;

		join(ap);
		indent(ap);  fprintf(ap->f, "(body)");
		ap->depth++; print_statement(ap, s->while_loop.body); ap->depth--;

		ap->depth--;
		break;
	case STMT_DO:
		ap->depth++;
		split(ap);

		indent(ap);  fprintf(ap->f, "(body)");
		ap->depth++; print_statement(ap, s->do_while_loop.body); ap->depth--;

		join(ap);
		indent(ap);  fprintf(ap->f, "(condition)");
		ap->depth++; print_expression(ap, s->do_while_loop.cond); ap->depth--;

		ap->depth--;
		break;
	case STMT_CLASS:
		ap->depth++;

		split(ap);

		indent(ap);  fprintf(ap->f, "(name)");
		ap->depth++;
		indent(ap);
		fprintf(ap->f, "(%s)", s->class.name);
		ap->depth--;

		for (size_t i = 0; i < s->class.num; i++) {
			if (i == s->class.num - 1) join(ap);
			print_statement(ap, s->class.body[i]);
		}

		ap->depth--;
		break;
	case STMT_IMPORT:
		ap->depth++;

		indent(ap);
		fprintf(ap->f, "import %s as %s", s->import.name->value,
		        s->import.as ? s->import.as->value : s->import.name->value);

		ap->depth--;
		break;
	case STMT_INVALID:
		fprintf(ap->f, "invalid statement; %s\n", s->tok->value);
		break;
	default:
		DOUT("unimplemented printer for statement of type %d (%s)",
		     s->type, statement_data[s->type].body);
	}
}

void
print_ast(FILE *f, struct statement **module)
{
	struct ASTPrinter *ap = oak_malloc(sizeof *ap);
	ap->f = f;
	ap->depth = 0;
	memset(ap->arm, 0, 2048 * sizeof (int));

	struct statement *s = NULL;

	indent(ap);
	fprintf(ap->f, "(root)");
	ap->depth++;
	split(ap);
	for (size_t i = 0; (s = module[i]); i++) {
		if (module[i + 1] == NULL) join(ap);
		print_statement(ap, s);
	}

	free(ap);
	fprintf(f, "\n");
}
