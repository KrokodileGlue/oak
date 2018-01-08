#ifndef VALUE_H
#define VALUE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "ktre.h"

#define NUM_ALLOCATABLE_VALUES 3

struct value {
	enum value_type {
		/* allocated */
		VAL_STR,
		VAL_ARRAY,
		VAL_REGEX,

		/* not allocated */
		VAL_NIL,
		VAL_INT,
		VAL_FLOAT,
		VAL_BOOL,
		VAL_FN
	} type;

	union {
		int64_t integer;
		double real;
		bool boolean;
		int64_t idx;
	};

	char *name;
};

#include "error.h"
#include "gc.h"

struct value_data {
	enum value_type type;
	char *body;
};

extern struct value_data value_data[];

struct value add_values(struct gc *gc, struct value l, struct value r);
struct value sub_values(struct gc *gc, struct value l, struct value r);
struct value mul_values(struct gc *gc, struct value l, struct value r);
struct value pow_values(struct gc *gc, struct value l, struct value r);
struct value div_values(struct gc *gc, struct value l, struct value r);
struct value mod_values(struct gc *gc, struct value l, struct value r);
struct value or_values(struct gc *gc, struct value l, struct value r);
struct value inc_value(struct gc *gc, struct value l);
struct value copy_value(struct gc *gc, struct value l);
struct value neg_value(struct gc *gc, struct value l);
struct value flip_value(struct gc *gc, struct value l);

struct value grow_array(struct gc *gc, struct value l, int r);
struct value pushback(struct gc *gc, struct value l, struct value r);

struct value cmp_values(struct gc *gc, struct value l, struct value r);
struct value value_less(struct gc *gc, struct value l, struct value r);
struct value value_more(struct gc *gc, struct value l, struct value r);
struct value value_len(struct gc *gc, struct value l);

bool is_truthy(struct gc *gc, struct value l);
void print_value(FILE *f, struct gc *gc, struct value val);

#endif
