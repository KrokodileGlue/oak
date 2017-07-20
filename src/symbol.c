#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "util.h"
#include "symbol.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "module.h"

static void add(struct symbolizer *si, struct symbol *sym);
static void symbolize(struct symbolizer *si, struct statement *stmt);
static struct symbol *new_symbol(struct token *tok);

static struct symbolizer *
new_symbolizer(struct oak *k)
{
	struct symbolizer *si = oak_malloc(sizeof *si);

	memset(si, 0, sizeof *si);
	si->k = k;
	si->es = new_error();

	return si;
}

void
free_symbol(struct symbol *sym)
{
	for (size_t i = 0; i < sym->num_children; i++) {
		free_symbol(sym->children[i]);
	}

	free(sym->children);
	free(sym);
}

void
symbolizer_free(struct symbolizer *si)
{
	free(si->es);
	free(si);
}

static char sym_str[][32] = {
	"function",
	"variable",
	"class",
	"global",
	"block",
	"argument",
	"module",
	"invalid"
};

static struct symbol *
new_symbol(struct token *tok)
{
	struct symbol *sym = oak_malloc(sizeof *sym);

	memset(sym, 0, sizeof *sym);
	sym->type = SYM_INVALID;
	sym->tok = tok;

	return sym;
}

static uint64_t
hash(char *d, size_t len)
{
	uint64_t hash = 5381;

	for (size_t i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + d[i];

	return hash;
}

static void
add(struct symbolizer *si, struct symbol *sym)
{
	if (!sym)
		return;

	struct symbol *symbol = si->symbol;
	symbol->children = oak_realloc(symbol->children,
	    sizeof symbol->children[0] * (symbol->num_children + 1));

	symbol->children[symbol->num_children++] = sym;
}

static struct symbol *
resolve(struct symbolizer *si, char *name)
{
	uint64_t h = hash(name, strlen(name));
	struct symbol *sym = si->symbol;

	while (sym) {
		for (size_t i = 0; i < sym->num_children; i++) {
			if (sym->children[i]->id == h
			    && !strcmp(sym->children[i]->name, name)) {
				return sym->children[i];
			}
		}

		sym = sym->parent;
	}

	return NULL;
}

static void
find(struct symbolizer *si, struct location loc, char *name)
{
	struct symbol *sym = resolve(si, name);
	if (!sym)
		error_push(si->es, loc, ERR_FATAL, "undeclared identifier");
}

static void
pop(struct symbolizer *si)
{
	if (si->symbol->parent)
		si->symbol = si->symbol->parent;
}

static void
push(struct symbolizer *si, struct symbol *sym)
{
	si->symbol = sym;
}

static void
block(struct symbolizer *si, struct statement *stmt)
{
	if (!stmt)
		return;

	struct symbol *sym = new_symbol(stmt->tok);
	sym->name = "*block*";
	sym->type = SYM_BLOCK;
	sym->parent = si->symbol;
	add(si, sym);

	push(si, sym);
	symbolize(si, stmt);
	pop(si);
}

static void
resolve_expr(struct symbolizer *si, struct expression *e)
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
			DOUT("unimplemented printer for expression of type %d", e->type);
		}
		break;
	case EXPR_VALUE:
		if (e->val->type == TOK_IDENTIFIER) {
			find(si, e->tok->loc, e->val->value);
		}
		break;
	default:
		break;
	}
}

static void
symbolize(struct symbolizer *si, struct statement *stmt)
{
	struct symbol *sym = new_symbol(stmt->tok);
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
			struct symbol *s = new_symbol(sym->tok);
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
			struct symbol *s = new_symbol(sym->tok);
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
	case STMT_IMPORT: {
		struct module *m = load_module(si->k, stmt->import.name->string, stmt->import.as->value);

		if (!m) {
			error_push(si->es, stmt->tok->loc, ERR_FATAL, "no such file");
			free(sym);
			return;
		}

		add(si, m->sym);

		free(sym);
		return;
	} break;
	default:
		free(sym);
		DOUT("unimplemented symbol visitor for statement of type %d (%s)",
		        stmt->type, statement_data[stmt->type].body);
		return;
	}

	sym->id = hash(sym->name, strlen(sym->name));
	add(si, sym);
}

bool
symbolize_module(struct module *m, struct oak *k)
{
	struct symbolizer *si = new_symbolizer(k);
	struct symbol *sym = new_symbol(m->tree[0]->tok);
	sym->type = SYM_MODULE;
	sym->name = m->name;
	sym->parent = si->symbol;
	sym->id = hash(m->name, strlen(m->name));
	sym->module = m;

	si->symbol = sym;

	size_t i = 0;
	while (m->tree[i]) {
		symbolize(si, m->tree[i++]);
	}

	m->sym = si->symbol;

	if (si->es->fatal) {
		error_write(si->es, stderr);
		symbolizer_free(si);
		return false;
	} else if (si->es->pending) {
		error_write(si->es, stderr);
	}

	symbolizer_free(si);
	m->stage = MODULE_STAGE_SYMBOLIZED;

	return true;
}

#define INDENT                             \
	fputc('\n', f);                    \
	for (size_t i = 0; i < depth; i++) \
		fprintf(f, "        ");

void
print_symbol(FILE *f, size_t depth, struct symbol *s)
{
	INDENT; fprintf(f, "<%s : %s>", s->name, sym_str[s->type]);
	INDENT; fprintf(f, "  num_children=%zd", s->num_children);
	INDENT; fprintf(f, "  id=%"PRIu64"", s->id);
//	fprintf(f, "<name=%s, type=%s, num_children=%zd, id=%"PRIu64">", s->name, sym_str[s->type], s->num_children, s->id);

	for (size_t i = 0; i < s->num_children; i++) {
		print_symbol(f, depth + 1, s->children[i]);
	}
}
