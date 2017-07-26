#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

struct list {
	uint64_t len;
	uint64_t *key;
	struct value *val;
};

struct list *new_list();
size_t list_index(struct list *l, void *key, size_t len);
size_t list_insert(struct list *l, void *key, size_t len, struct value val);
struct value *list_lookup(struct list *l, void *key, size_t len);

#endif
