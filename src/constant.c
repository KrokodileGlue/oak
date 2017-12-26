#include "constant.h"
#include "util.h"
#include <string.h>

struct constant_table *
new_constant_table()
{
	struct constant_table *t = oak_malloc(sizeof *t);
	memset(t, 0, sizeof *t);
	return t;
}

int
constant_table_add(struct constant_table *t, struct value v)
{
	if (t->allocated <= t->num) {
		t->allocated *= 2;
		t->val = oak_realloc(t->val, t->allocated * sizeof *t->val);
	}
	t->val[t->num] = v;
	return t->num++;
}
