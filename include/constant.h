#ifndef CONSTANT_H
#define CONSTANT_H

#include <stdlib.h>
#include "value.h"

struct constant_table {
	struct value *val;
	size_t num;
	size_t allocated;
};

struct constant_table *new_constant_table();
void free_constant_table(struct constant_table *t);
int constant_table_add(struct constant_table *t, struct value v);
void print_constant_table(FILE *f, struct gc *gc, struct constant_table *ct);

#endif
