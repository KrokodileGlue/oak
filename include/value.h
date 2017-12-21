#ifndef VALUE_H
#define VALUE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

struct value {
	enum value_type {
		VAL_NIL,
		VAL_INT,
		VAL_FLOAT,
		VAL_STR,
		VAL_BOOL
	} type;

	union {
		int64_t integer;
		double real;
		bool boolean;
		int64_t idx;
	};
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
struct value div_values(struct gc *gc, struct value l, struct value r);

bool is_value_true(struct gc *gc, struct value l);
void print_value(FILE *f, struct gc *gc, struct value val);

#endif
