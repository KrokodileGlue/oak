#include "ast-printer.h"
#include "keyword.h"
#include "util.h"
#include "token.h"
#include "assert.h"

struct ASTPrinter {
	FILE *f;
	size_t depth;
	int arm[2048]; /* this is probably enough. */
};

static void split(struct ASTPrinter *ap)
{
	ap->arm[ap->depth - 1] = 1;
}

static void join(struct ASTPrinter *ap)
{
	ap->arm[ap->depth - 1] = 0;
}

static void indent(struct ASTPrinter *ap)
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

static void print_expression(struct ASTPrinter *ap, struct Expression *e)
{
	indent(ap);

	if (e->type == EXPR_OPERATOR) {
		struct Operator *op = e->operator;

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
			break;
		case OP_MEMBER:
			fprintf(ap->f,"(member of )");
			break;
		case OP_INVALID:
			fprintf(stderr, "An invalid node was encountered. This should never happen.\n");
			assert(false);
			break;
		}
	} else {
		switch (e->value->type) {
		case TOK_IDENTIFIER:
			fprintf(ap->f, "(identifier %s)", e->value->value);
			break;
		case TOK_INTEGER:
			fprintf(ap->f, "(integer %zd)", e->value->integer);
			break;
		case TOK_STRING:
			fprintf(ap->f, "(string \"%s\")", e->value->string);
			break;
		case TOK_FLOAT:
			fprintf(ap->f, "(float %f)", e->value->floating);
			break;
		case TOK_BOOL:
			fprintf(ap->f, "(bool %s)", e->value->boolean ? "true" : "false");
			break;
		default:
			fprintf(stderr, "impossible value token %d\n", e->type);
			assert(false);
		}
	}
}

static void print_statement(struct ASTPrinter *ap, struct Statement *s)
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
			ap->depth++;
			print_expression(ap, s->var_decl.init[i]);
			ap->depth--;
		}

		ap->depth--;
		break;
	default:
		fprintf(ap->f, "unimplemented statement printer\n");
	}
}

void print_ast(FILE *f, struct Statement **module)
{
	struct ASTPrinter *ap = oak_malloc(sizeof *ap);
	ap->f = f;
	ap->depth = 0;
	memset(ap->arm, 0, 2048 * sizeof (int));

	struct Statement *s = NULL;

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
