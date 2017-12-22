#define _GNU_SOURCE
#include <string.h>

#include "gc.h"

static int64_t
bmp_alloc(uint64_t *bmp, int64_t slots){
	slots /= 64;

	for (int64_t i = 0; i < slots; i++){
		if (*bmp == 0xFFFFFFFFFFFFFFFFLL){
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

int64_t gc_alloc(struct gc *gc, enum value_type type)
{
	int64_t idx = bmp_alloc(gc->bmp, 64);
	return idx;
}
