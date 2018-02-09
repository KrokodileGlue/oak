#include <string.h>
#include <assert.h>

#include "util.h"
#include "array.h"

struct array *
new_array()
{
	struct array *a = oak_malloc(sizeof *a);
	memset(a, 0, sizeof *a);

	a->alloc = 16;
	a->v = oak_malloc(16 * sizeof *a->v);

	for (size_t i = 0; i < 16; i++)
		a->v[i] = NIL;

	return a;
}

void
free_array(struct array *a)
{
	free(a->v);
	free(a);
}

void
array_push(struct array *a, struct value r)
{
	grow_array(a, a->len + 1);
	a->v[a->len++] = r;
}

void
grow_array(struct array *a, size_t size)
{
	assert(size < SIZE_MAX / 2);

	if (size >= a->alloc * 2) {
		a->v = oak_realloc(a->v, (size + 1) * sizeof *a->v);

		for (size_t i = a->alloc; i < size; i++)
			a->v[i] = NIL;

		a->alloc += size;
		return;
	}

	while (size >= a->alloc) {
		a->v = oak_realloc(a->v, a->alloc * 2 * sizeof *a->v);

		for (size_t i = a->alloc; i < a->alloc * 2; i++)
			a->v[i] = NIL;

		a->alloc *= 2;
	}
}

void
array_insert(struct array *a, size_t idx, struct value r)
{
	grow_array(a, a->len + 1);
	if (idx >= a->alloc) grow_array(a, idx + 1);
	memmove(a->v + idx + 1, a->v + idx, labs(a->len - idx) * sizeof *a->v);
	a->v[idx] = r;
	a->len++;
}
