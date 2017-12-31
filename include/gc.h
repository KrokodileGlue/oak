#ifndef GC_H
#define GC_H

#include <stdint.h>
#include "value.h"

struct gc {
	int64_t slot[NUM_ALLOCATABLE_VALUES];
	uint64_t *bmp[NUM_ALLOCATABLE_VALUES];

	bool debug;

	/* The actual value structures. */
	char **str;
};

struct gc *new_gc();
int64_t gc_alloc(struct gc *gc, enum value_type type);

#endif
