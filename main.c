#include "include/oak.h"
#include <stdlib.h>

char *
strclone(const char *str)
{
	char *ret = malloc(strlen(str) + 1);
	strcpy(ret, str);
	return ret;
}

void
chop_extension(char *a)
{
	while (*a != '.' && *a) a++;
	*a = 0;
}

char *
process_arguments(struct oak *k, int argc, char **argv)
{
	char *path = NULL;
	k->talkative = false;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-np")) k->talkative = true;
		if (!strcmp(argv[i], "-pi")) k->print_input = true;
		if (!strcmp(argv[i], "-pt")) k->print_tokens = true;
		if (!strcmp(argv[i], "-pa")) k->print_ast = true;
		if (!strcmp(argv[i], "-ps")) k->print_symbol_table = true;
		if (!strcmp(argv[i], "-pc")) k->print_code = true;
		if (!strcmp(argv[i], "-pg")) k->print_gc = true;
		if (!strcmp(argv[i], "-pv")) k->print_vm = true;
		if (!strcmp(argv[i], "-d"))  k->debug = true;
		if (!strcmp(argv[i], "-p"))  k->print_everything = true;
		if (!strcmp(argv[i], "-e")) {
			if (k->eval) {
				printf("oak: invalid options; received multiple -e\n");
				exit(EXIT_FAILURE);
			}

			i++;
			k->eval = strclone(argv[i]);
			continue;
		}

		if (argv[i][0] != '-')  {
			if (path) {
				printf("oak: invalid options; received multiple input files; '%s' and '%s'\n", path, argv[i]);
				exit(EXIT_FAILURE);
			} else {
				path = argv[i];
			}
		}
	}

	k->print_anything = (k->print_input || k->print_tokens || k->print_ast || k->print_symbol_table || k->print_code || k->print_everything);
	if (k->print_everything) k->print_code = true;
	if (k->print_everything) k->print_gc = true;
	if (k->print_everything) k->print_vm = true;
	k->talkative = !k->talkative;

	if (!path && !k->eval) {
		printf("oak: invalid options; did not receive an input file\n");
		exit(EXIT_FAILURE);
	}

	return path;
}

char *
load_file(const char *path)
{
	char *b = NULL;
	FILE *f = fopen(path, "r");
	if (!f) return NULL;

	if (fseek(f, 0L, SEEK_END) == 0) {
		long len = ftell(f);
		if (len == -1) goto err;
		b = malloc(len + 1);
		if (fseek(f, 0L, SEEK_SET)) goto err;
		len = fread(b, 1, len, f);
		if (ferror(f))  goto err;
		else b[len++] = 0;
	}

	fclose(f);
	return b;

 err:
	if (f) fclose(f);
	return NULL;
}

int
main(int argc, char **argv)
{
	oak *k = oak_new();
	char *path = process_arguments(k, argc, argv), *name = NULL;
	struct module *m = NULL;

	if (path) {
		name = strclone(path);
		chop_extension(name);
		m = oak_load_module(k, path);
	}

	if (k->eval && m) {
		oak_load_child2(k, m, k->eval);
		oak_print_value(stdout, m->gc, k->stack[k->sp - 1]);
		putchar('\n');
	} else if (k->eval) {
		struct module *e = oak_load_module2(k, k->eval);

		if (e) {
			print_value(stdout, e->gc, k->stack[k->sp - 1]);
			putchar('\n');
		}
	}

	oak_print_modules(k);
	oak_free(k);
	free(name);

	return 0;
}
