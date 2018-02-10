#ifndef TABLE_H
#define TABLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define TABLE_SIZE 32

struct table {
	struct bucket {
		uint64_t *h;
		char **key;
		struct value *val;
		size_t len;
	} bucket[TABLE_SIZE];
};

struct table *new_table();
struct table *copy_table(struct table *t);
void free_table(struct table *t);
struct value table_lookup(struct table *t, char *key);
struct value table_add(struct table *t, char *key, struct value v);

#endif
