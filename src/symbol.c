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

struct symbolizer *
new_symbolizer(struct oak *k)
{
	struct symbolizer *si = oak_malloc(sizeof *si);

	memset(si, 0, sizeof *si);
	si->k = k;
	si->r = new_reporter();

	return si;
}

void
free_symbol(struct symbol *sym)
{
	for (size_t i = 0; i < sym->num_children; i++) {
		/*
		 * Module symbols are free'd by themselves.
		 */
		if (sym->children[i]->type != SYM_MODULE)
			free_symbol(sym->children[i]);
	}

	free(sym->children);
	free(sym);
}

void
symbolizer_free(struct symbolizer *si)
{
	error_clear(si->r);
	free(si->scope_stack);
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
	sym->scope = -1;

	return sym;
}

static void
push_scope(struct symbolizer *si, int scope)
{
	si->scope_stack = realloc(si->scope_stack, sizeof si->scope_stack[0] * (si->scope_pointer + 1));
	si->scope_stack[si->scope_pointer++] = scope;
}

static void
pop_scope(struct symbolizer *si)
{
	si->scope_pointer--;
}

static void
inc_variable_count(struct symbol *sym)
{
	while (sym) {
		sym->num_variables++;
		sym = sym->parent;
	}
}

// TODO: make this faster.
struct symbol *
find_from_scope(struct symbol *sym, int scope)
{
	if (sym->scope == scope) return sym;

	for (size_t i = 0; i < sym->num_children; i++) {
		struct symbol *ret = find_from_scope(sym->children[i], scope);
		if (ret) return ret;
	}

	return NULL;
}

static void
add(struct symbolizer *si, struct symbol *sym)
{
	if (!sym)
		return;

	if (sym->type == SYM_VAR || sym->type == SYM_ARGUMENT) {
		inc_variable_count(si->symbol);
	}

	struct symbol *symbol = si->symbol;
	symbol->children = oak_realloc(symbol->children,
	    sizeof symbol->children[0] * (symbol->num_children + 1));

	symbol->children[symbol->num_children++] = sym;
}

struct symbol *
resolve(struct symbol *sym, char *name)
{
	uint64_t h = hash(name, strlen(name));

	while (sym) {
		if (sym->id == h && !strcmp(sym->name, name)) {
			return sym;
		}

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
	struct symbol *sym = resolve(si->symbol, name);
	if (!sym)
		error_push(si->r, loc, ERR_FATAL, "undeclared identifier");
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
push_block(struct symbolizer *si, struct statement *stmt)
{
	if (!stmt)
		return;

	struct symbol *sym = new_symbol(stmt->tok);
	sym->name = "*block*";
	sym->type = SYM_BLOCK;
	sym->parent = si->symbol;
	sym->scope  = si->scope_stack[si->scope_pointer - 1];
	stmt->scope = si->scope_stack[si->scope_pointer - 1];

	add(si, sym);

	push(si, sym);
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
	sym->scope = si->scope_stack[si->scope_pointer - 1];
	stmt->scope = si->scope_stack[si->scope_pointer - 1];
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
		case OPTYPE_PREFIX: case OPTYPE_POSTFIX:
			resolve_expr(si, e->a);
			break;
		case OPTYPE_BINARY:
			resolve_expr(si, e->a);
			resolve_expr(si, e->b);

			break;
		case OPTYPE_TERNARY:
			resolve_expr(si, e->a);
			resolve_expr(si, e->b);
			resolve_expr(si, e->c);
			break;
		default:
			DOUT("unimplemented symbolizer for operator of type %d", e->operator->type);
			break;
		}
		break;
	case EXPR_VALUE:
		if (e->val->type == TOK_IDENTIFIER) {
			find(si, e->tok->loc, e->val->value);
		} break;
	case EXPR_SUBSCRIPT:
		resolve_expr(si, e->a);
		resolve_expr(si, e->b);
		break;
	case EXPR_FN_CALL:
		resolve_expr(si, e->a);
		for (size_t i = 0; i < e->num; i++)
			resolve_expr(si, e->args[i]);

		break;
	case EXPR_LIST:
		for (size_t i = 0; i < e->num; i++) {
			resolve_expr(si, e->args[i]);
		}
		break;
	case EXPR_LIST_COMPREHENSION:
		push_block(si, e->s);

		symbolize(si, e->s);
		resolve_expr(si, e->a);
		resolve_expr(si, e->b);
		if (e->c) resolve_expr(si, e->c);

		pop(si);
		break;
	default:
		DOUT("unimplemented symbolizer for expression of type %d", e->type);
		break;
	}
}

static void
symbolize(struct symbolizer *si, struct statement *stmt)
{
	struct symbol *sym = new_symbol(stmt->tok);
	sym->scope  = si->scope_stack[si->scope_pointer - 1];
	stmt->scope = si->scope_stack[si->scope_pointer - 1];
	sym->parent = si->symbol;

	switch (stmt->type) {
	case STMT_CLASS:
		si->scope++;
		sym->name = stmt->class.name;
		sym->type = SYM_CLASS;

		push(si, sym);

		if (stmt->class.parent_name) {
			struct symbol *parent = resolve(si->symbol, stmt->class.parent_name->value);

			if (parent) {
				if (parent->type != SYM_CLASS) {
					error_push(si->r, stmt->tok->loc, ERR_FATAL, "class inherits from non-inheritable symbol");
					error_push(si->r, parent->tok->loc, ERR_NOTE, "previous declaration here");
				}

				sym->parent = parent;
			}
		}

		for (size_t i = 0; i < stmt->class.num; i++) {
			symbolize(si, stmt->class.body[i]);
		}

		pop(si);

		break;
	case STMT_FN_DEF:
		push_scope(si, ++si->scope);
		stmt->scope = si->scope_stack[si->scope_pointer - 1];

		sym->name = stmt->fn_def.name;
		sym->type = SYM_FN;
		/*
		 * We have to set the id early to support recursive
		 * function calls n stuff.
		 */
		sym->id = hash(sym->name, strlen(sym->name));
		sym->scope = si->scope_stack[si->scope_pointer - 1];

		push(si, sym);

		for (size_t i = 0; i < stmt->fn_def.num; i++) {
			struct symbol *s = new_symbol(sym->tok);
			s->type = SYM_ARGUMENT;
			s->name = stmt->fn_def.args[i]->value;
			s->id = hash(s->name, strlen(s->name));
			add(si, s);
		}

		push_scope(si, ++si->scope);
		block(si, stmt->fn_def.body);
		pop_scope(si);

		pop(si);
		pop_scope(si);

		break;
	case STMT_VAR_DECL: {
#define VARDECL(STATEMENT)                                                                                            \
		for (size_t i = 0; i < STATEMENT->var_decl.num; i++) {                                                \
			if (STATEMENT->var_decl.init) resolve_expr(si, STATEMENT->var_decl.init[i]);                  \
			struct symbol *redefinition = resolve(si->symbol, STATEMENT->var_decl.names[i]->value);       \
			                                                                                              \
			if (redefinition) {                                                                           \
				error_push(si->r, STATEMENT->tok->loc, ERR_FATAL, "redeclaration of identifier '%s'", \
				           STATEMENT->var_decl.names[i]->value);                                      \
				error_push(si->r, redefinition->tok->loc, ERR_NOTE, "previously defined here");       \
			}                                                                                             \
			                                                                                              \
			struct symbol *s = new_symbol(sym->tok);                                                      \
			s->type = SYM_VAR;                                                                            \
			s->name = STATEMENT->var_decl.names[i]->value;                                                \
			s->id = hash(s->name, strlen(s->name));                                                       \
			s->scope = -1;                                                                                \
			add(si, s);                                                                                   \
		}                                                                                                     \

		VARDECL(stmt);
		free(sym);
		return;
	} break;
	case STMT_BLOCK: {
		si->scope++;
		push_scope(si, si->scope);

		si->scope++;
		stmt->scope = si->scope_stack[si->scope_pointer - 1];
		sym->name = "*block*";
		sym->type = SYM_BLOCK;
		sym->scope = si->scope_stack[si->scope_pointer - 1];

		push(si, sym);

		for (size_t i = 0; i < stmt->block.num; i++)
			symbolize(si, stmt->block.stmts[i]);

		pop(si);
	} break;

	case STMT_IF_STMT: {
		push_scope(si, si->scope_stack[si->scope_pointer - 1]);

		resolve_expr(si, stmt->if_stmt.cond);
		si->scope++; block(si, stmt->if_stmt.then);
		si->scope++; block(si, stmt->if_stmt.otherwise);
		pop_scope(si);

		free(sym);
		return;
	} break;

	case STMT_EXPR:
		resolve_expr(si, stmt->expr);
		free(sym);
		return;
	case STMT_PRINT: case STMT_PRINTLN:
		for (size_t i = 0; i < stmt->print.num; i++) {
			resolve_expr(si, stmt->print.args[i]);
		}

		free(sym);
		return;
	case STMT_IMPORT: {
		struct module *m = load_module(si->k, stmt->import.name->string, stmt->import.as->value);

		if (!m) {
			error_push(si->r, stmt->tok->loc, ERR_FATAL, "could not load module");
			free(sym);
			return;
		}

		add(si, m->sym);

		free(sym);
		return;
	} break;

	case STMT_WHILE: {
		push_scope(si, si->scope);

		resolve_expr(si, stmt->while_loop.cond);
		si->scope++;
		block(si, stmt->while_loop.body);
		pop_scope(si);

		free(sym);
		return;
	}

	case STMT_FOR_LOOP: { // TODO: figure out the scopes for the rest of these things.
		si->scope++;
		push_scope(si, si->scope);
		push_block(si, stmt);
		stmt->scope = si->scope_stack[si->scope_pointer - 1];

		if (!stmt->for_loop.c) {
			inc_variable_count(si->symbol);
		}

		if (!stmt->for_loop.b && !stmt->for_loop.c) {
			struct symbol *s = new_symbol(sym->tok);
			s->type = SYM_VAR;
			s->name = "i";
			s->id = hash(s->name, strlen(s->name));
			s->scope = -1;
			add(si, s);
		}

		symbolize(si, stmt->for_loop.a);
		if (stmt->for_loop.b) resolve_expr(si, stmt->for_loop.b);
		if (stmt->for_loop.c) resolve_expr(si, stmt->for_loop.c);
		symbolize(si, stmt->for_loop.body);

		pop(si);
		pop_scope(si);

		free(sym);
		return;
	} break;

	case STMT_DO: {
		block(si, stmt->do_while_loop.body);
		resolve_expr(si, stmt->do_while_loop.cond);
		free(sym);
		return;
	} break;

	case STMT_RET: {
		resolve_expr(si, stmt->ret.expr);
		free(sym);
		return;
	} break;

	case STMT_NULL: {
		free(sym);
		return;
	}
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

	if (!m->tree || !m->tree[0]) {
		error_push(si->r, (struct location){0,0,0,0}, ERR_FATAL, "empty modules are not permitted");
		return false;
	}

	struct symbol *sym = new_symbol(m->tree[0]->tok);
	sym->type = SYM_MODULE;
	sym->name = m->name;
	sym->parent = si->symbol;
	sym->id = hash(m->name, strlen(m->name));
	sym->module = m;
	sym->scope = 0;
	push_scope(si, 0);

	si->scope = sym->scope;
	si->symbol = sym;

	size_t i = 0;
	while (m->tree[i]) {
		symbolize(si, m->tree[i++]);
	}

	m->sym = si->symbol;

	if (si->r->fatal) {
		error_write(si->r, stderr);
		symbolizer_free(si);
		return false;
	} else if (si->r->pending) {
		error_write(si->r, stderr);
	}

	symbolizer_free(si);
	m->stage = MODULE_STAGE_SYMBOLIZED;

	return true;
}

#define INDENT                             \
	fputc('\n', f);                    \
	for (size_t i = 0; i < depth; i++) \
		fprintf(f, "  ");

void
print_symbol(FILE *f, size_t depth, struct symbol *s)
{
	INDENT; fprintf(f, "<%s : %s>", s->name, sym_str[s->type]);

	if (s->num_children) {
		INDENT; fprintf(f, "  num_children=%zu", s->num_children);
	}

	if (s->num_variables) {
		INDENT; fprintf(f, "  num_variables=%zu", s->num_variables);
	}

	INDENT; fprintf(f, "  address=%zu", s->address);
	INDENT; fprintf(f, "  scope=%d", s->scope);

	INDENT; fprintf(f, "  id=%"PRIu64"", s->id);
//	fprintf(f, "<name=%s, type=%s, num_children=%zd, id=%"PRIu64">", s->name, sym_str[s->type], s->num_children, s->id);

	for (size_t i = 0; i < s->num_children; i++) {
		print_symbol(f, depth + 1, s->children[i]);
	}
}
