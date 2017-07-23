#ifndef VALUE_H
#define VALUE_H

#include <stdlib.h>
#include <stdint.h>

struct value {
	enum value_type {
		VAL_INT,
		VAL_FLOAT,
		VAL_STR,
		VAL_BOOL
	} type;

	union {
		int64_t integer;

		struct {
			char *text;
			size_t len;
		} str;
	};
};

struct valueData {
	enum value_type type;
	char *body;
};

extern struct valueData value_data[];

struct value add_values(struct value l, struct value r);

#endif
