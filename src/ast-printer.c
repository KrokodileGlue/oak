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
			fprintf(ap->f, "(string \"%s\")", e->value->value);
			break;
		case TOK_FLOAT:
			fprintf(ap->f, "(float %f)", e->value->floating);
			break;
		default:
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

		print_expression(ap, s->if_stmt.cond);

		if (s->if_stmt.otherwise) {
			print_statement(ap, s->if_stmt.then);
			join(ap);
			print_statement(ap, s->if_stmt.otherwise);
		} else {
			join(ap);
			print_statement(ap, s->if_stmt.then);
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
	default:
		fprintf(ap->f, "cannot yet print statement.\n");
	}
}

void print_ast(FILE *f, struct Statement **module)
{
	struct ASTPrinter *ap = oak_malloc(sizeof *ap);
	ap->f = f;
	ap->depth = 0;
	memset(ap->arm, 0, 2048 * sizeof (int));

	struct Statement *s = NULL;

	for (size_t i = 0; (s = module[i]); i++) {
		s = module[i];
		print_statement(ap, s);
	}

	fprintf(f, "\n");
}
