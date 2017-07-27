#ifndef VALUE_H
#define VALUE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "error.h"
#include "vm.h"
#include "list.h"

struct value {
	enum value_type {
		VAL_NIL,
		VAL_INT,
		VAL_FLOAT,
		VAL_STR,
		VAL_BOOL,
		VAL_LIST
	} type;

	union {
		int64_t integer;
		double real;
		bool boolean;

		struct {
			char *text;
			size_t len;
		} str;

		struct list *list;
	};
};

struct valueData {
	enum value_type type;
	char *body;
};

extern struct valueData value_data[];

struct value add_values(struct vm *vm, struct value l, struct value r);
struct value sub_values(struct vm *vm, struct value l, struct value r);
struct value mul_values(struct vm *vm, struct value l, struct value r);
struct value div_values(struct vm *vm, struct value l, struct value r);
struct value mod_values(struct vm *vm, struct value l, struct value r);

struct value cmp_values(struct vm *vm, struct value l, struct value r);
struct value and_values(struct vm *vm, struct value l, struct value r);

struct value inc_value(struct vm *vm, struct value l);
struct value dec_value(struct vm *vm, struct value l);
struct value len_value(struct vm *vm, struct value l);
struct value flip_value(struct vm *vm, struct value l);

struct value is_less_than_value(struct vm *vm, struct value l, struct value r);
struct value is_more_than_value(struct vm *vm, struct value l, struct value r);

bool is_value_true(struct vm *vm, struct value l);
void print_value(FILE *f, struct value val);

#endif
