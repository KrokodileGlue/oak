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
	default:
		DOUT("unimplemented value adder thing for value of type %d", l.type);
		assert(false);
	}

	return ret;
}
