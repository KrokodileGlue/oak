#ifndef VALUE_H
#define VALUE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "ktre.h"

#define NUM_ALLOCATABLE_VALUES 4

struct value {
	enum value_type {
		/* allocated */
		VAL_STR,
		VAL_ARRAY,
		VAL_REGEX,
		VAL_TABLE,

		/* not allocated */
		VAL_NIL,
		VAL_INT,
		VAL_FLOAT,
		VAL_BOOL,
		VAL_FN,
		VAL_UNDEF,
		VAL_ERR
	} type;

	union {
		double real;
		bool boolean;

		struct {
			int64_t idx;

			/*
			 * The number of times to execute the
			 * substitution in a regular expression
			 * substitution with /e
			 */
			unsigned char e;
		};

		struct {
			uint8_t num_args;
			uint16_t module;
			int64_t integer;
		};
	};

	char *name;
};

#define BOOL(X) ((struct value){ VAL_BOOL, { .boolean = (X) }, NULL})
#define INT(X) ((struct value){ VAL_INT, { .integer = (X) }, NULL})
#define ERR(X) ((struct value){ VAL_ERR, { .integer = 0 }, (X)})
#define NIL ((struct value){ VAL_NIL, { .integer = 0 }, NULL})

#include "error.h"
#include "gc.h"
#include "array.h"

struct gc;

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
struct value copy_value(struct gc *gc, struct value l);
struct value rev_value(struct gc *gc, struct value l);
struct value sort_value(struct gc *gc, struct value l);
struct value max_value(struct gc *gc, struct value l);
struct value uc_value(struct gc *gc, struct value l);
struct value lc_value(struct gc *gc, struct value l);
struct value ucfirst_value(struct gc *gc, struct value l);
struct value lcfirst_value(struct gc *gc, struct value l);
struct value inc_value(struct value l);
struct value dec_value(struct value l);
struct value neg_value(struct value l);
struct value abs_value(struct value l);
struct value flip_value(struct gc *gc, struct value l);

struct value cmp_values(struct gc *gc, struct value l, struct value r);
struct value less_values(struct gc *gc, struct value l, struct value r);
struct value leq_values(struct gc *gc, struct value l, struct value r);
struct value geq_values(struct gc *gc, struct value l, struct value r);
struct value more_values(struct gc *gc, struct value l, struct value r);
struct value value_len(struct gc *gc, struct value l);
struct value value_translate(struct gc *l, struct gc *r, struct value v);
struct value int_value(struct gc *gc, struct value l);
struct value float_value(struct gc *gc, struct value l);

bool is_truthy(struct gc *gc, struct value l);
void print_value(FILE *f, struct gc *gc, struct value val);
char *show_value(struct gc *gc, struct value val);
void print_debug(struct gc *gc, struct value l);
#endif
