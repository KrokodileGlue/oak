#include <string.h>

#include "list.h"
#include "util.h"
#include "value.h"

struct list *
new_list()
{
	struct list *l = oak_malloc(sizeof *l);
	memset(l, 0, sizeof *l);
	return l;
}

uint64_t
list_index(struct list *l, void *key, size_t len)
{
	uint64_t h = hash(key, len);

	for (uint64_t i = 0; i < l->len; i++) {
		if (l->key[i] == h)
			return i;
	}

	return (uint64_t)-1;
}

uint64_t
list_insert(struct list *l, void *key, size_t len, struct value val)
{
	if (list_index(l, key, len) != (uint64_t)-1)
		return (uint64_t)-1;

	uint64_t h = hash(key, len);
	l->val = realloc(l->val, sizeof l->val[0] * (l->len + 1));
	l->key = realloc(l->key, sizeof l->key[0] * (l->len + 1));

	l->key[l->len] = h;
	l->val[l->len] = val;
	l->len++;

	return h;
}

struct value *
list_lookup(struct list *l, void *key, size_t len)
{
	uint64_t h = hash(key, len);
	if (h == (uint64_t)-1) return NULL;
	return &l->val[list_index(l, key, len)];
}
