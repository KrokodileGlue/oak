#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "token.h"

struct Symbolizer *mksymbolizer(struct Statement **module)
{
	struct Symbolizer *si = oak_malloc(sizeof *si);

	memset(si, 0, sizeof *si);
	si->m = module; /* the statement stream */

	return si;
}

char sym_str[][32] = {
	"function",
	"variable",
	"class",
	"global",
	"block",
	"argument",
	"invalid"
};

struct Symbol *mksym(struct Token *tok)
{
	struct Symbol *sym = oak_malloc(sizeof *sym);

	memset(sym, 0, sizeof *sym);
	sym->type = SYM_INVALID;
	sym->tok = tok;

	return sym;
}

uint64_t hash(char *d, size_t len)
{
	uint64_t hash = 5381;

	for (size_t i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + d[i];

	return hash;
}

struct Symbol *symbolize_stmt(struct Symbolizer *si, struct Statement *stmt)
{
	struct Symbol *sym = mksym(stmt->tok);
	sym->parent = si->symbol;

	switch (stmt->type) {
	case STMT_CLASS:
		sym->name = stmt->class.name;
		sym->type = SYM_CLASS;

		si->symbol = sym;

		sym->children = oak_realloc(sym->children, sizeof sym->children[0] * (sym->num_children + 1));

		for (size_t i = 0; i < stmt->class.num; i++) {
			struct Symbol *ret = symbolize_stmt(si, stmt->class.body[i]);

			if (ret) {
				sym->children[sym->num_children++] = ret;
			} else
				free(ret);
		}

		si->symbol = sym->parent;

		break;
	case STMT_FN_DEF:
		sym->name = stmt->fn_def.name;
		sym->type = SYM_FN;

		si->symbol = sym;

		sym->children = oak_realloc(sym->children,
		    sizeof sym->children[0] * (stmt->fn_def.num + sym->num_children + 1));
		struct Symbol **children = sym->children;

		for (size_t i = 0; i < stmt->fn_def.num; i++) {
			children[sym->num_children + i] = mksym(sym->tok);
			children[sym->num_children + i]->type = SYM_ARGUMENT;
			children[sym->num_children + i]->name = stmt->fn_def.args[i]->value;
		}
		sym->num_children += stmt->fn_def.num;

		struct Symbol *ret = symbolize_stmt(si, stmt->fn_def.body);
		if (ret) {
			sym->children[sym->num_children++] = ret;
		} else
			free(ret);

		si->symbol = sym->parent;

		break;
	case STMT_VAR_DECL: {
		struct Symbol *symbol = si->symbol;
		symbol->children = oak_realloc(symbol->children,
		    sizeof symbol->children[0] * (stmt->var_decl.num + symbol->num_children + 1));
		struct Symbol **children = symbol->children;

		for (size_t i = 0; i < stmt->var_decl.num; i++) {
			children[symbol->num_children + i] = mksym(sym->tok);
			children[symbol->num_children + i]->type = SYM_VAR;
			children[symbol->num_children + i]->name = stmt->var_decl.names[i]->value;
		}
		symbol->num_children += stmt->var_decl.num;

		free(sym);
		return NULL;
	} break;
	case STMT_BLOCK: {
		sym->name = "*block*";
		sym->type = SYM_BLOCK;

		si->symbol = sym;

		sym->children = oak_realloc(sym->children,
		    sizeof sym->children[0] * (stmt->block.num + sym->num_children + 1));

		size_t valid_children = 0;
		for (size_t i = 0; i < stmt->block.num; i++) {
			struct Symbol *ret = symbolize_stmt(si, stmt->block.stmts[i]);
			if (ret) {
				sym->children[sym->num_children + i] = ret;
				valid_children++;
			}
			else     free(ret);
		}
		sym->num_children += valid_children;

		si->symbol = sym->parent;
	} break;
	case STMT_PRINT:
		free(sym);
		return NULL;
		break;
	default:
		free(sym);
		fprintf(stderr, "\nunimplemented symbol visitor for statement of type %d (%s)",
		        stmt->type, statement_data[stmt->type].body);
		return NULL;
	}

	return sym;
}

struct Symbol *symbolize(struct Symbolizer *si)
{
	struct Symbol *sym = mksym(si->m[0]->tok);
	sym->type = SYM_GLOBAL;
	sym->name = "*global*";

	si->symbol = sym;

	while (si->m[si->i]) {
		sym->children = oak_realloc(sym->children,
		                            sizeof sym->children[0] * (sym->num_children + 1));
		struct Symbol *ret = symbolize_stmt(si, si->m[si->i++]);
		if (ret) {
			sym->children[sym->num_children++] = ret;
		} else
			free(ret);
	}

	return sym;
}

void print_symbol(FILE *f, size_t depth, struct Symbol *s)
{
	fputc('\n', f);

	for (size_t i = 0; i < depth; i++)
		fprintf(f, "    ");

	fprintf(f, "<name=%s, type=%s, num_children=%zd>", s->name, sym_str[s->type], s->num_children);

	for (size_t i = 0; i < s->num_children; i++) {
		print_symbol(f, depth + 1, s->children[i]);
	}
}
