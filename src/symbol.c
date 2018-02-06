#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#include "util.h"
#include "symbol.h"
#include "token.h"
#include "lexer.h"
#include "parse.h"
#include "module.h"

static void add(struct symbolizer *si, struct symbol *sym);
static void symbolize(struct symbolizer *si, struct statement *stmt);
static struct symbol *new_symbol(struct token *tok, struct symbol *_sym);

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
			free_symbol(sym->children[i]);
	}

	free(sym->label);
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

static char *sym_str[] = {
	"function",
	"variable",
	"global",
	"block",
	"argument",
	"module",
	"invalid"
};

static struct symbol *
new_symbol(struct token *tok, struct symbol *_sym)
{
	struct symbol *sym = oak_malloc(sizeof *sym);

	memset(sym, 0, sizeof *sym);
	sym->type = SYM_INVALID;
	sym->tok = tok;
	sym->scope = -1;
	if (_sym) sym->module = _sym->module;
	sym->next = -1;
	sym->last = -1;
	sym->address = -1;

	return sym;
}

void
set_next(struct symbol *sym, int next)
{
	if (sym->scope == 0) return;
	if (sym->next < next && sym->next != -1) return;
	sym->next = next;

	for (size_t i = 0; i < sym->num_children; i++)
		set_next(sym->children[i], next);
}

void
set_last(struct symbol *sym, int last)
{
	if (sym->scope == 0) return;
	if (sym->last < last && sym->last != -1) return;
	sym->last = last;

	for (size_t i = 0; i < sym->num_children; i++)
		set_last(sym->children[i], last);
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
	if (!sym) return;

	if (get_builtin(sym->name)) {
		error_push(si->r, sym->tok->loc, ERR_FATAL,
		           "redeclaration of builtin function `%s' as %s",
		           sym->name, sym_str[sym->type]);
		free_symbol(sym);
		return;
	}

	if (sym->type == SYM_VAR || sym->type == SYM_ARGUMENT)
		inc_variable_count(si->symbol);

	struct symbol *symbol = si->symbol;
	symbol->children = oak_realloc(symbol->children,
	    sizeof symbol->children[0] * (symbol->num_children + 1));

	if (sym->next < 0) sym->next = symbol->next;
	if (sym->last < 0) sym->last = symbol->last;

	symbol->children[symbol->num_children++] = sym;
}

struct symbol *
resolve(struct symbol *sym, char *name)
{
	if (!name || !*name) return NULL;
	uint64_t h = hash(name, strlen(name));

	while (sym) {
		if (sym->id == h && !strcmp(sym->name, name))
			return sym;

		for (size_t i = 0; i < sym->num_children; i++) {
			if (sym->children[i]->id == h
			    && !strcmp(sym->children[i]->name, name)) {
				return sym->children[i];
			}
		}

		if (sym->type == SYM_FN) {
			while (sym->type != SYM_MODULE) sym = sym->parent;
			continue;
		}

		sym = sym->parent;
	}

	return NULL;
}

static void
find(struct symbolizer *si, struct location loc, char *name)
{
	struct symbol *sym = resolve(si->symbol, name);
	if (!sym && strcmp(name, "nil"))
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

	struct symbol *sym = new_symbol(stmt->tok, si->symbol);
	sym->name = "*block*";
	sym->type = SYM_BLOCK;
	sym->parent = si->symbol;
	sym->fp = si->fp;
	sym->scope  = si->scope_stack[si->scope_pointer - 1];
	stmt->scope = si->scope_stack[si->scope_pointer - 1];

	add(si, sym);
	push(si, sym);
}

static void
push_block_expr(struct symbolizer *si, struct expression *expr)
{
	assert(expr);

	struct symbol *sym = new_symbol(expr->tok, si->symbol);
	sym->name = "*block*";
	sym->type = SYM_BLOCK;
	sym->parent = si->symbol;
	sym->fp = si->fp;
	sym->scope  = si->scope_stack[si->scope_pointer - 1];

	add(si, sym);
	push(si, sym);
}

static void
block(struct symbolizer *si, struct statement *stmt)
{
	if (!stmt)
		return;

	struct symbol *sym = new_symbol(stmt->tok, si->symbol);
	sym->name = "*block*";
	sym->type = SYM_BLOCK;
	sym->parent = si->symbol;
	sym->fp = si->fp;
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
	if (!e) return;

	switch (e->type) {
	case EXPR_OPERATOR:
		switch (e->operator->type) {
		case OPTYPE_PREFIX: case OPTYPE_POSTFIX:
			resolve_expr(si, e->a);
			break;

		case OPTYPE_BINARY:
			resolve_expr(si, e->a);
			if (e->operator->name != OP_PERIOD)
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
		if (e->val->type == TOK_IDENTIFIER)
			find(si, e->tok->loc, e->val->value);
		break;

	case EXPR_SUBSCRIPT:
		resolve_expr(si, e->a);
		resolve_expr(si, e->b);
		break;

	case EXPR_SLICE:
		resolve_expr(si, e->a);
		resolve_expr(si, e->b);
		resolve_expr(si, e->c);
		break;

	case EXPR_FN_CALL:
		resolve_expr(si, e->a);
		for (size_t i = 0; i < e->num; i++)
			resolve_expr(si, e->args[i]);
		break;

	case EXPR_LIST:
		for (size_t i = 0; i < e->num; i++)
			resolve_expr(si, e->args[i]);
		break;

	case EXPR_LIST_COMPREHENSION:
		push_scope(si, ++si->k->scope);
		push_block(si, e->s);

		symbolize(si, e->s);

		if (!e->c) {
			struct symbol *imp = new_symbol(e->tok, si->symbol);
			imp->type = SYM_VAR;
			imp->name = "_";
			imp->id = hash(imp->name, strlen(imp->name));
			imp->scope = -1;

			si->symbol->imp = true;
			add(si, imp);
		}

		resolve_expr(si, e->a);
		resolve_expr(si, e->b);
		resolve_expr(si, e->c);

		pop_scope(si);
		pop(si);
		break;

	case EXPR_FN_DEF:
		symbolize(si, e->s);
		break;

	case EXPR_MAP: {
		resolve_expr(si, e->b);

		push_scope(si, ++si->k->scope);
		push_block_expr(si, e);

		struct symbol *s = new_symbol(e->tok, si->symbol);
		s->type = SYM_VAR;
		s->name = "_";
		s->id = hash(s->name, strlen(s->name));
		s->scope = -1;
		add(si, s);

		resolve_expr(si, e->a);

		pop_scope(si);
		pop(si);
	} break;

	case EXPR_EVAL:
	case EXPR_REGEX:
	case EXPR_GROUP:
		break;

	case EXPR_TABLE:
	case EXPR_BUILTIN:
		for (size_t i = 0; i < e->num; i++)
			resolve_expr(si, e->args[i]);
		break;

	case EXPR_MATCH:
		resolve_expr(si, e->a);

		push_scope(si, ++si->k->scope);
		push_block_expr(si, e);
		si->symbol->imp = true;

		struct symbol *s = new_symbol(e->tok, si->symbol);
		s->type = SYM_VAR;
		s->name = "_";
		s->id = hash(s->name, strlen(s->name));
		s->scope = -1;
		add(si, s);

		for (size_t i = 0; i < e->num; i++) {
			resolve_expr(si, e->args[i]);
			resolve_expr(si, e->match[i]);
			symbolize(si, e->bodies[i]);
		}

		pop(si);
		pop_scope(si);
		break;

	default:
		DOUT("unimplemented symbolizer for expression of type %d", e->type);
		assert(false);
	}
}

static void
symbolize(struct symbolizer *si, struct statement *stmt)
{
	if (!stmt) return;

	struct symbol *sym = new_symbol(stmt->tok, si->symbol);
	sym->scope  = si->scope_stack[si->scope_pointer - 1];
	stmt->scope = si->scope_stack[si->scope_pointer - 1];
	sym->parent = si->symbol;
	sym->fp     = si->fp;
	resolve_expr(si, stmt->condition);

	switch (stmt->type) {
	case STMT_FN_DEF: {
		si->fp++;

		struct symbol *redefinition = resolve(si->symbol, stmt->fn_def.name);
		if (redefinition) {
			error_push(si->r, stmt->tok->loc, ERR_FATAL, "redeclaration of identifier `%s' as function",
			           stmt->fn_def.name);
			error_push(si->r, redefinition->tok->loc, ERR_NOTE, "previously defined here");
		}

		push_scope(si, ++si->k->scope);
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
			struct symbol *s = new_symbol(sym->tok, si->symbol);
			s->type = SYM_ARGUMENT;
			s->name = stmt->fn_def.args[i]->value;
			s->id = hash(s->name, strlen(s->name));
			si->symbol->num_arguments++;
			add(si, s);
		}

		push_scope(si, ++si->k->scope);
		block(si, stmt->fn_def.body);
		pop_scope(si);

		pop(si);
		pop_scope(si);
		si->fp--;
	} break;

	case STMT_VAR_DECL: {
#define VARDECL(STATEMENT)                                                                                            \
		for (size_t i = 0; i < STATEMENT->var_decl.num; i++) {                                                \
			if (STATEMENT->var_decl.init) resolve_expr(si, STATEMENT->var_decl.init[i]);                  \
			struct symbol *redefinition = resolve(si->symbol, STATEMENT->var_decl.names[i]->value);       \
			                                                                                              \
			if (redefinition && redefinition->parent->scope == si->scope_stack[si->scope_pointer - 1]) {  \
				error_push(si->r, STATEMENT->tok->loc, ERR_FATAL, "redeclaration of identifier `%s'", \
				           STATEMENT->var_decl.names[i]->value);                                      \
				error_push(si->r, redefinition->tok->loc, ERR_NOTE, "previously defined here");       \
			}                                                                                             \
			                                                                                              \
			struct symbol *s = new_symbol(sym->tok, si->symbol);                                          \
			s->type = SYM_VAR;                                                                            \
			s->name = STATEMENT->var_decl.names[i]->value;                                                \
			s->id = hash(s->name, strlen(s->name));                                                       \
			s->scope = -1;                                                                                \
			s->parent = si->symbol;                                                                       \
			s->global = si->scope_pointer == 1;                                                           \
			add(si, s);                                                                                   \
		}                                                                                                     \

		VARDECL(stmt);
		free(sym);
		return;
	} break;

	case STMT_BLOCK: {
		push_scope(si, ++si->k->scope);

		stmt->scope = si->k->scope;
		sym->name = "*block*";
		sym->type = SYM_BLOCK;
		sym->scope = si->k->scope;

		push(si, sym);

		for (size_t i = 0; i < stmt->block.num; i++)
			symbolize(si, stmt->block.stmts[i]);

		pop(si);
		pop_scope(si);
	} break;

	case STMT_IF_STMT: {
		push_scope(si, si->scope_stack[si->scope_pointer - 1]);

		resolve_expr(si, stmt->if_stmt.cond);
		si->k->scope++; block(si, stmt->if_stmt.then);
		si->k->scope++; block(si, stmt->if_stmt.otherwise);
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
		struct module *m = load_module(si->k,
		                               NULL,
		                               load_file(stmt->import.name->string),
		                               stmt->import.name->string,
		                               stmt->import.as->value,
		                               NULL, 0);

		if (!m) {
			error_push(si->r, stmt->tok->loc, ERR_FATAL,
			           "could not load module");
			free(sym);
			return;
		}

		add(si, m->sym);
		free(sym);
		return;
	} break;

	case STMT_WHILE: {
		push_scope(si, si->k->scope);

		resolve_expr(si, stmt->while_loop.cond);
		si->k->scope++;
		block(si, stmt->while_loop.body);
		pop_scope(si);

		free(sym);
		return;
	}

	case STMT_FOR_LOOP: { // TODO: figure out the scopes for the rest of these things.
		push_scope(si, ++si->k->scope);
		push_block(si, stmt);
		stmt->scope = si->scope_stack[si->scope_pointer - 1];

		if (stmt->for_loop.a->type == STMT_VAR_DECL)
			inc_variable_count(si->symbol);

		if (!stmt->for_loop.b || !stmt->for_loop.c) {
			inc_variable_count(si->symbol);
			inc_variable_count(si->symbol);
			inc_variable_count(si->symbol);
		}

		if (!stmt->for_loop.b && !stmt->for_loop.c) {
			si->symbol->imp = true;

			struct symbol *s = new_symbol(sym->tok, si->symbol);
			s->type = SYM_VAR;
			s->name = "_";
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

	case STMT_DIE:
		resolve_expr(si, stmt->expr);
		free(sym);
		return;

	case STMT_LABEL:
		sym->type = SYM_LABEL;
		sym->name = stmt->label;
		sym->scope = si->k->scope;
		sym->parent = si->symbol;
		sym->global = si->scope_pointer == 1;
		break;

	case STMT_GOTO:
	case STMT_LAST:
	case STMT_NEXT:
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
symbolize_module(struct module *m, struct oak *k, struct symbol *parent)
{
	struct symbolizer *si = new_symbolizer(k);

	if (!m->tree || !m->tree[0]) {
		error_push(si->r, (struct location){ m->text, m->path, 0, 0 }, ERR_FATAL, "empty modules are not permitted");
		error_write(si->r, stderr);
		symbolizer_free(si);
		return false;
	}

	struct symbol *sym;

	if (parent) {
		sym = parent;
	} else {
		sym = new_symbol(m->tree[0]->tok, NULL);
		sym->type = SYM_MODULE;
		sym->name = m->name;
		sym->parent = si->symbol;
		sym->fp = si->fp;
		sym->id = hash(m->name, strlen(m->name));
		sym->module = m;
		sym->scope = 0;
	}

	push_scope(si, sym->scope);
	si->symbol = sym;

	if (parent) {
		push_scope(si, ++k->scope);
		push_block(si, m->tree[0]);
		si->symbol->module = m;
		si->symbol->next = si->symbol->parent->next;
		si->symbol->last = si->symbol->parent->last;
	}

	for (size_t i = 0; m->tree[i]; i++)
		symbolize(si, m->tree[i]);

	m->sym = sym;

	if (si->r->fatal) {
		error_write(si->r, stderr);
		if (!m->child)
			free_symbol(si->symbol);
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

	if (s->num_arguments) {
		INDENT; fprintf(f, "  num_arguments=%zu", s->num_arguments);
	}

	INDENT; fprintf(f, "  address=%zu", s->address);
	INDENT; fprintf(f, "  scope=%d", s->scope);
	INDENT; fprintf(f, "  global=%s", s->global ? "true" : "false");
	INDENT; fprintf(f, "  module=%s", s->module->name);
	INDENT; fprintf(f, "  next=%d", s->next);
	INDENT; fprintf(f, "  last=%d", s->last);
	INDENT; fprintf(f, "  imp=%d", s->imp);

	INDENT; fprintf(f, "  id=%"PRIu64"", s->id);
//	fprintf(f, "<name=%s, type=%s, num_children=%zd, id=%"PRIu64">", s->name, sym_str[s->type], s->num_children, s->id);

	for (size_t i = 0; i < s->num_children; i++) {
		print_symbol(f, depth + 1, s->children[i]);
	}
}
