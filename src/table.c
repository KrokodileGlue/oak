#include <string.h>
#include "table.h"
#include "value.h"
#include "util.h"

struct table *
new_table()
{
	struct table *t = malloc(sizeof *t);
	memset(t, 0, sizeof *t);
	return t;
}

void
free_table(struct table *t)
{
	for (size_t i = 0; i < TABLE_SIZE; i++) {
		free(t->bucket[i].h);
		free(t->bucket[i].key);
		free(t->bucket[i].val);
	}

	free(t);
}

struct value
table_add(struct table *t, char *key, struct value v)
{
	uint64_t h = hash(key, strlen(key));

	int idx = h % TABLE_SIZE;
	struct bucket *b = t->bucket + idx;

	for (size_t i = 0; i < b->len; i++) {
		if (b->h[i] == h) {
			b->val[i] = v;
			return v;
		}
	}

	b->h   = realloc(b->h,   sizeof b->h[0]   * (b->len + 1));
	b->val = realloc(b->val, sizeof b->val[0] * (b->len + 1));
	b->key = realloc(b->key, sizeof b->key[0] * (b->len + 1));

	b->h  [b->len] = h;
	b->key[b->len] = key;
	b->val[b->len] = v;

	b->len++;
	return b->val[b->len - 1];
}

struct value
table_lookup(struct table *t, char *key)
{
	uint64_t h = hash(key, strlen(key));
	int idx = h % TABLE_SIZE;

	struct bucket *b = t->bucket + idx;

	for (size_t i = 0; i < b->len; i++)
		if (b->h[i] == h)
			return b->val[i];

	return (struct value){ VAL_NIL, { 0 }, NULL };
}
