#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "constant.h"
#include "util.h"

struct constant_table *
new_constant_table()
{
	struct constant_table *t = oak_malloc(sizeof *t);
	memset(t, 0, sizeof *t);
	t->allocated = 1;
	t->val = oak_malloc(sizeof *t->val);
	return t;
}

void
free_constant_table(struct constant_table *t)
{
	free(t->val);
	free(t);
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
		fprintf(f, "\n[%3zu : %9s] ", i, value_data[ct->val[i].type].body);
		switch (ct->val[i].type) {
		case VAL_STR:
			print_escaped_string(f, gc->str[ct->val[i].idx],
			                     strlen(gc->str[ct->val[i].idx]));
			break;

		case VAL_INT:
			fprintf(f, "%"PRId64, ct->val[i].integer);
			break;

		case VAL_FLOAT:
			fprintf(f, "%f", ct->val[i].real);
			break;

		case VAL_NIL:
			fprintf(f, "nil");
			break;

		case VAL_ARRAY:
			for (unsigned int j = 0; j < gc->arrlen[ct->val[i].idx]; j++)
				print_value(f, gc, gc->array[ct->val[i].idx][j]);
			break;

		case VAL_BOOL:
			fprintf(f, "%s", ct->val[i].boolean ? "true" : "false");
			break;

		case VAL_FN:
			fprintf(f, "%"PRId64, ct->val[i].integer);
			break;

		case VAL_REGEX:
			fprintf(f, "%p", (void *)&gc->regex[ct->val[i].idx]);
			break;

		default:
			DOUT("unimplemented constant printer for value of type %s",
			     value_data[ct->val[i].type].body);
			assert(false);
		}
	}
}
