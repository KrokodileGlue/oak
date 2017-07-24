#include <string.h>
#include <assert.h>
#include "util.h"

#include "value.h"

struct valueData value_data[] = {
	{ VAL_INT,   "integer" },
	{ VAL_FLOAT, "float" },
	{ VAL_STR,   "string" }
};

struct value
add_values(struct value l, struct value r)
{
	if (l.type != r.type) {
		// TODO: runtime error reporting
		printf("l.type = %d\n", l.type);
		printf("r.type = %d\n", r.type);
		assert(false);
	}

	struct value ret;
	ret.type = l.type;

	switch (l.type) {
	case VAL_INT:
		ret.integer = l.integer + r.integer;
		break;
	case VAL_FLOAT:
		ret.real = l.real + r.real;
		break;
	case VAL_STR:
		ret.str.text = oak_malloc(strlen(l.str.text) + strlen(r.str.text) + 1);
		strcpy(ret.str.text, l.str.text);
		strcpy(ret.str.text + strlen(l.str.text), r.str.text);
		ret.str.len = strlen(ret.str.text);
		break;
	default:
		DOUT("unimplemented value adder thing for value of type %d", l.type);
		assert(false);
	}

	return ret;
}

struct value sub_values(struct value l, struct value r)
{
	if (l.type != r.type) {
		// TODO: runtime error reporting
		printf("l.type = %d\n", l.type);
		printf("r.type = %d\n", r.type);
		assert(false);
	}

	struct value ret;
	ret.type = l.type;

	switch (l.type) {
	case VAL_INT:
		ret.integer = l.integer - r.integer;
		break;
	case VAL_FLOAT:
		ret.real = l.real - r.real;
		break;
	default:
		DOUT("unimplemented value adder thing for value of type %d", l.type);
		assert(false);
	}

	return ret;
}

struct value mul_values(struct value l, struct value r)
{
	if (l.type != r.type) {
		// TODO: runtime error reporting
		printf("l.type = %d\n", l.type);
		printf("r.type = %d\n", r.type);
		assert(false);
	}

	struct value ret;
	ret.type = l.type;

	switch (l.type) {
	case VAL_INT:
		ret.integer = l.integer * r.integer;
		break;
	case VAL_FLOAT:
		ret.real = l.real * r.real;
		break;
	default:
		DOUT("unimplemented value adder thing for value of type %d", l.type);
		assert(false);
	}

	return ret;
}

struct value div_values(struct value l, struct value r)
{
	if (l.type != r.type) {
		// TODO: runtime error reporting
		printf("l.type = %d\n", l.type);
		printf("r.type = %d\n", r.type);
		assert(false);
	}

	struct value ret;
	ret.type = l.type;

	switch (l.type) {
	case VAL_INT:
		ret.integer = l.integer / r.integer;
		break;
	case VAL_FLOAT:
		ret.real = l.real / r.real;
		break;
	default:
		DOUT("unimplemented value adder thing for value of type %d", l.type);
		assert(false);
	}

	return ret;
}
