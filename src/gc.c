#define _GNU_SOURCE
#include <string.h>
#include <assert.h>

#include "util.h"
#include "gc.h"

struct gc *
new_gc()
{
	struct gc *gc = oak_malloc(sizeof *gc);
	memset(gc, 0, sizeof *gc);
	return gc;
}

static int64_t
bmp_alloc(uint64_t *bmp, int64_t slots)
{
	slots /= 64;

	for (int64_t i = 0; i < slots; i++) {
		if (*bmp == 0xFFFFFFFFFFFFFFFFLL) {
			/* go to the next bitmap if this one is full */
			bmp++;
			continue;
		}

		int pos = ffsll(~*bmp) - 1;
		*bmp |= 1LL << pos;

		return i * 64 + pos;
	}

	return -1;
}

int64_t
gc_alloc(struct gc *gc, enum value_type type)
{
	int64_t idx = bmp_alloc(gc->bmp[type], gc->slot[type]);

	if (idx == -1) {
		gc->slot[type] += 64;
		gc->bmp[type] = oak_realloc(gc->bmp[type],
		                            (gc->slot[type] / 64) * sizeof *gc->bmp[type]);
		memset(gc->bmp[type] + (gc->slot[type] - 64) / 64, 0,
		       (gc->slot[type] / 64) * sizeof *gc->bmp[type]);

		switch (type) {
		case VAL_STR:
			gc->str = oak_realloc(gc->str,
			                      gc->slot[type] * sizeof *gc->str);
			break;
		default:
			DOUT("unimplemented gc allocator");
			assert(false);
		}

		idx = bmp_alloc(gc->bmp[type], gc->slot[type]);
	}

	return idx;
}
