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

void
free_gc(struct gc *gc)
{
	/* Kinda like a final gc pass. */

	uint64_t *bmp = gc->bmp[VAL_STR];
	for (int64_t i = 0; i < gc->slot[VAL_STR] / 64; i++) {
		for (int j = 0; j < 64; j++) {
			if (*bmp == 0LL) break;
			int pos = ffsll(*bmp) - 1;
			*bmp ^= 1LL << pos;
			if (gc->str[i * 64 + pos])
				free(gc->str[i * 64 + pos]);
		}

		bmp++;
	}

	bmp = gc->bmp[VAL_ARRAY];
	for (int64_t i = 0; i < gc->slot[VAL_ARRAY] / 64; i++) {
		for (int j = 0; j < 64; j++) {
			if (*bmp == 0LL) break;
			int pos = ffsll(*bmp) - 1;
			*bmp ^= 1LL << pos;
			if (gc->array[i * 64 + pos])
				free(gc->array[i * 64 + pos]);
		}

		bmp++;
	}

	bmp = gc->bmp[VAL_REGEX];
	for (int64_t i = 0; i < gc->slot[VAL_REGEX] / 64; i++) {
		for (int j = 0; j < 64; j++) {
			if (*bmp == 0LL) break;
			int pos = ffsll(*bmp) - 1;
			*bmp ^= 1LL << pos;
			if (gc->regex[i * 64 + pos])
				ktre_free(gc->regex[i * 64 + pos]);
		}

		bmp++;
	}

	bmp = gc->bmp[VAL_TABLE];
	for (int64_t i = 0; i < gc->slot[VAL_TABLE] / 64; i++) {
		for (int j = 0; j < 64; j++) {
			if (*bmp == 0LL) break;
			int pos = ffsll(*bmp) - 1;
			*bmp ^= 1LL << pos;
			if (gc->table[i * 64 + pos])
				free_table(gc->table[i * 64 + pos]);
		}

		bmp++;
	}

	for (int i = 0; i < NUM_ALLOCATABLE_VALUES; i++)
		if (gc->bmp[i]) free(gc->bmp[i]);

	if (gc->array)  free(gc->array);
	if (gc->str)    free(gc->str);
	if (gc->arrlen) free(gc->arrlen);
	if (gc->regex)  free(gc->regex);

	free(gc);
}

static int64_t
bmp_alloc(uint64_t *bmp, int64_t slots)
{
	slots /= 64;

	for (int64_t i = 0; i < slots; i++) {
		if (*bmp == 0xFFFFFFFFFFFFFFFFLL) {
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
		                            ((gc->slot[type] / 64) + 1) * sizeof *gc->bmp[type]);
		memset(&gc->bmp[type][(gc->slot[type] - 64) / 64], 0,
		       sizeof *gc->bmp[type]);

		switch (type) {
		case VAL_STR:
			gc->str = oak_realloc(gc->str,
			                      gc->slot[type] * sizeof *gc->str);
			break;

		case VAL_ARRAY:
			gc->array = oak_realloc(gc->array,
			                      gc->slot[type] * sizeof *gc->array);

			gc->arrlen = oak_realloc(gc->arrlen,
			                      gc->slot[type] * sizeof *gc->arrlen);
			break;

		case VAL_REGEX:
			gc->regex = oak_realloc(gc->regex,
			                        gc->slot[type] * sizeof *gc->regex);
			break;

		case VAL_TABLE:
			gc->table = oak_realloc(gc->table,
			                      gc->slot[type] * sizeof *gc->table);
			break;

		default:
			DOUT("unimplemented gc allocator");
			assert(false);
		}

		idx = bmp_alloc(gc->bmp[type], gc->slot[type]);
	}

	return idx;
}
