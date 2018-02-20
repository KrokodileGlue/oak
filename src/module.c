#include "module.h"
#include "util.h"
#include "token.h"
#include "symbol.h"
#include "vm.h"
#include "lexer.h"
#include "parse.h"

struct module *
new_module(char *text, char *path)
{
	struct module *m = oak_malloc(sizeof *m);

	memset(m, 0, sizeof *m);
	m->path = strclone(path);
	m->stage = MODULE_STAGE_EMPTY;
	m->gc = new_gc();
	m->text = text;

	return m;
}

void
free_module(struct module *m)
{
	if (m->k->print_gc) DOUT("freeing module %s", m->name);

	if (m->stage >= MODULE_STAGE_LEXED)      token_clear(m->tok);
	if (m->stage >= MODULE_STAGE_PARSED)     free_ast(m->tree);
	if (m->stage >= MODULE_STAGE_SYMBOLIZED)
		if (!m->child) free_symbol(m->sym);

	if (!m->child) free_gc(m->gc);
	if (!m->child) free_constant_table(m->ct);
	if (!m->child) free(m->global);

	free_vm(m->vm);
	free(m->text);
	free(m->name);
	free(m->path);
	free(m->code);

	free(m);
}

void
add_module(struct oak *k, struct module *m)
{
	k->modules = oak_realloc(k->modules, (k->num + 1) * sizeof *k->modules);
	m->id = k->num;
	k->modules[k->num++] = m;
}

void
print_modules(struct oak *k)
{
	if (k->print_anything) {
		for (size_t i = 0; i < k->num; i++) {
			struct module *m = k->modules[i];

			if (i != 0) fputc('\n', stderr);
			fprintf(stderr, "============================== module `%s' ==============================", m->name);

			if (m->stage >= MODULE_STAGE_EMPTY
			    && (k->print_input || k->print_everything))
				fprintf(stderr, "\n%s", m->text);

			if (m->stage >= MODULE_STAGE_LEXED
			    && (k->print_tokens || k->print_everything))
				token_write(m->tok, stderr);

			if (m->stage >= MODULE_STAGE_PARSED
			    && (k->print_ast || k->print_everything))
				print_ast(stderr, m->tree);

			if (m->stage >= MODULE_STAGE_SYMBOLIZED
			    && (k->print_symbol_table || k->print_everything)) {
				if (m->child)
					fprintf(stderr,
					        "\nnot printing symbol table - module is a child of `%s'.",
					        m->parent->name);
				else print_symbol(stderr, 0, m->sym);
			}

			if (m->stage >= MODULE_STAGE_COMPILED
			    && (k->print_code || k->print_everything)) {
				if (m->child)
					fprintf(stderr,
					        "\nnot printing constant table - module is a child of `%s'.",
					        m->parent->name);
				else print_constant_table(stderr, m->gc, m->ct);
				print_code(stderr, m->code, m->num_instr);
			}
		}

		fputc('\n', stderr);
	}
}

struct module *
load_module(struct oak *k, struct symbol *parent, char *text,
            char *path, char *name, struct vm *vm, int stack_base)
{
	if (!text) {
		fprintf(stderr, "Could not load file %s\n", path);
		return NULL;
	}

	for (size_t i = 0; i < k->num; i++)
		if (!strcmp(k->modules[i]->path, path) && strncmp(name, "*eval", 5))
			return free(text), k->modules[i];

	struct module *m = new_module(text, path);

	if (vm) {
		m->child = true;
		m->parent = vm->m;
		m->global = vm->m->global;
		free_gc(m->gc);
		m->gc = vm->gc;
	}

	m->name = strclone(name);
	m->k = k;
	if (k->print_gc) m->gc->debug = true;

	add_module(k, m);

	if (!tokenize(m)) return NULL;
	if (!parse(m)) return NULL;
	if (!symbolize_module(m, k, parent)) return NULL;
	while (m->sym->parent) m->sym = m->sym->parent;
	if (!compile(m, vm ? vm->m->ct : NULL, parent ? parent : m->sym,
	             k->print_code, !!vm, vm ? stack_base : -1)) return NULL;

	if (vm) {
		struct constant_table *ct = vm->ct;
		struct instruction *c = vm->code;
		struct module *m2 = vm->m;
		int ip = vm->ip;

		vm->ct = m->ct;
		vm->code = m->code;
		vm->m = m;
		m->vm = vm;

		if (!k->debug) execute(m->vm, 0);
		m->vm = NULL;

		vm->code = c;
		vm->m = m2;
		vm->ip = ip;
		vm->ct = ct;
	} else {
		m->vm = new_vm(m, k, k->print_vm);
		push_frame(m->vm);
		if (!k->debug) execute(m->vm, 0);
		m->vm->fp = 0;
	}

	return m;
}
