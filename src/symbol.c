#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "util.h"
#include "symbol.h"
#include "token.h"

struct Symbolizer *mksymbolizer(struct Statement **module)
{
	struct Symbolizer *si = oak_malloc(sizeof *si);

	memset(si, 0, sizeof *si);
	si->m = module; /* the statement stream */
	si->es = error_new();

	return si;
}

static char sym_str[][32] = {
	"function",
	"variable",
	"class",
	"global",
	"block",
	"argument",
	"invalid"
};

static struct Symbol *mksym(struct Token *tok)
{
	struct Symbol *sym = oak_malloc(sizeof *sym);

	memset(sym, 0, sizeof *sym);
	sym->type = SYM_INVALID;
	sym->tok = tok;

	return sym;
}

static uint64_t hash(char *d, size_t len)
{
	uint64_t hash = 5381;

	for (size_t i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + d[i];

	return hash;
}

static void add(struct Symbolizer *si, struct Symbol *sym)
{
	if (!sym)
		return;

	struct Symbol *symbol = si->symbol;
	symbol->children = oak_realloc(symbol->children,
	    sizeof symbol->children[0] * (symbol->num_children + 1));

	symbol->children[symbol->num_children++] = sym;
}

static bool resolve(struct Symbolizer *si, struct Location loc, char *name)
{
	uint64_t h = hash(name, strlen(name));
	struct Symbol *sym = si->symbol;

	while (sym) {
		for (size_t i = 0; i < sym->num_children; i++) {
			if (sym->children[i]->id == h) {
				return true;
			}
		}
		sym = sym->parent;
	}

	error_push(si->es, loc, ERR_FATAL, "undeclared identifier");
	return false;
}

static void pop(struct Symbolizer *si)
{
	si->symbol = si->symbol->parent;
}

static void push(struct Symbolizer *si, struct Symbol *sym)
{
	si->symbol = sym;
}

static void symbolize(struct Symbolizer *si, struct Statement *stmt);

static void block(struct Symbolizer *si, struct Statement *stmt)
{
	if (!stmt)
		return;

	struct Symbol *sym = mksym(stmt->tok);
	sym->name = "*block*";
	sym->type = SYM_BLOCK;
	sym->parent = si->symbol;

	push(si, sym);
	symbolize(si, stmt);
	pop(si);
}

static void resolve_expr(struct Symbolizer *si, struct Expression *e)
{
	switch (e->type) {
	case EXPR_OPERATOR:
		switch (e->operator->type) {
		case OP_BINARY:
			resolve_expr(si, e->a);
			resolve_expr(si, e->b);
			break;
		case OP_FN_CALL:
			resolve_expr(si, e->a);
			for (size_t i = 0; i < e->num; i++)
				resolve_expr(si, e->args[i]);
			break;
		default:
			fprintf(stderr, "unimplemented expression thing\n");
		}
		break;
	case EXPR_VALUE:
		if (e->val->type == TOK_IDENTIFIER) {
			fprintf(stderr, "");
			resolve(si, e->tok->loc, e->val->value);
		}
		break;
	default:
		break;
	}
}

static void symbolize(struct Symbolizer *si, struct Statement *stmt)
{
	struct Symbol *sym = mksym(stmt->tok);
	sym->parent = si->symbol;

	switch (stmt->type) {
	case STMT_CLASS:
		sym->name = stmt->class.name;
		sym->type = SYM_CLASS;

		push(si, sym);

		for (size_t i = 0; i < stmt->class.num; i++) {
			symbolize(si, stmt->class.body[i]);
		}

		pop(si);

		break;
	case STMT_FN_DEF:
		sym->name = stmt->fn_def.name;
		sym->type = SYM_FN;

		push(si, sym);

		for (size_t i = 0; i < stmt->fn_def.num; i++) {
			struct Symbol *s = mksym(sym->tok);
			s->type = SYM_ARGUMENT;
			s->name = stmt->fn_def.args[i]->value;
			s->id = hash(s->name, strlen(s->name));
			add(si, s);
		}

		symbolize(si, stmt->fn_def.body);
		pop(si);

		break;
	case STMT_VAR_DECL: {
		for (size_t i = 0; i < stmt->var_decl.num; i++) {
			struct Symbol *s = mksym(sym->tok);
			s->type = SYM_VAR;
			s->name = stmt->var_decl.names[i]->value;
			s->id = hash(s->name, strlen(s->name));
			add(si, s);
		}

		free(sym);
		return;
	} break;
	case STMT_BLOCK: {
		sym->name = "*block*";
		sym->type = SYM_BLOCK;

		push(si, sym);

		for (size_t i = 0; i < stmt->block.num; i++)
			symbolize(si, stmt->block.stmts[i]);

		pop(si);
	} break;
	case STMT_IF_STMT: {
		block(si, stmt->if_stmt.then);
		block(si, stmt->if_stmt.otherwise);

		free(sym);
		return;
	} break;
	case STMT_EXPR:
		resolve_expr(si, stmt->expr);
	case STMT_PRINT: /* fall through */
		free(sym);
		return;
		break;
	default:
		free(sym);
		fprintf(stderr, "\nunimplemented symbol visitor for statement of type %d (%s)",
		        stmt->type, statement_data[stmt->type].body);
		return;
	}

	sym->id = hash(sym->name, strlen(sym->name));
	add(si, sym);
}

struct Symbol *symbolize_module(struct Symbolizer *si)
{
	struct Symbol *sym = mksym(si->m[0]->tok);
	sym->type = SYM_GLOBAL;
	sym->name = "*global*";

	si->symbol = sym;

	while (si->m[si->i]) {
		symbolize(si, si->m[si->i++]);
	}

	return sym;
}

#define INDENT                             \
	fputc('\n', f);                    \
	for (size_t i = 0; i < depth; i++) \
		fprintf(f, "        ");

void print_symbol(FILE *f, size_t depth, struct Symbol *s)
{
	INDENT; fprintf(f, "<%s : %s>", s->name, sym_str[s->type]);
	INDENT; fprintf(f, "  num_children=%zd", s->num_children);
	INDENT; fprintf(f, "  id=%"PRIu64"", s->id);
//	fprintf(f, "<name=%s, type=%s, num_children=%zd, id=%"PRIu64">", s->name, sym_str[s->type], s->num_children, s->id);

	for (size_t i = 0; i < s->num_children; i++) {
		print_symbol(f, depth + 1, s->children[i]);
	}
}
