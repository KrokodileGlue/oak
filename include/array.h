#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>
#include "value.h"
#include "gc.h"

struct array {
	struct value *v;
	unsigned len;
	size_t alloc;
};

struct array *new_array();
struct value array_pop(struct array *a);
struct value array_shift(struct array *a);
void free_array(struct array *a);
void grow_array(struct array *a, size_t size);
void array_push(struct array *a, struct value r);
void array_insert(struct array *a, size_t idx, struct value r);

#endif
