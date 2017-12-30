#include "constant.h"
#include "util.h"
#include <string.h>

struct constant_table *
new_constant_table()
{
	struct constant_table *t = oak_malloc(sizeof *t);
	memset(t, 0, sizeof *t);
	t->allocated = 1;
	t->val = oak_malloc(sizeof *t->val);
	return t;
}

int
constant_table_add(struct constant_table *t, struct value v)
{
	while (t->allocated <= t->num) {
		t->allocated *= 2;
		t->val = oak_realloc(t->val, t->allocated * sizeof *t->val);
	}

	t->val[t->num] = v;
	return t->num++;
}

void
print_constant_table(FILE *f, struct gc *gc, struct constant_table *ct)
{
	for (size_t i = 0; i < ct->num; i++) {
		fprintf(f, "\n[%zu : %s] ", i, value_data[ct->val[i].type].body);
		print_value(f, gc, ct->val[i]);
	}
}
