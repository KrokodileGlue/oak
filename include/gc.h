#ifndef GC_H
#define GC_H

#include <stdint.h>
#include "value.h"
#include "table.h"

struct gc {
	int64_t slot[NUM_ALLOCATABLE_VALUES];
	uint64_t *bmp[NUM_ALLOCATABLE_VALUES];

	bool debug;

	/* The actual value structures. */
	char **str;
	struct value **array;
	unsigned int *arrlen;
	struct ktre **regex;
	struct table **table;
};

struct gc *new_gc();
void free_gc(struct gc *gc);
int64_t gc_alloc(struct gc *gc, enum value_type type);

#endif
