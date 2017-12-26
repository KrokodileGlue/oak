#ifndef GC_H
#define GC_H

#include <stdint.h>
#include "value.h"

struct gc {
	uint64_t *strbmp;
	char **str;
};

struct gc *new_gc();
int64_t gc_alloc(struct gc *gc, enum value_type type);

#endif
