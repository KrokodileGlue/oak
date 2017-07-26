#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "util.h"
#include "value.h"

struct valueData value_data[] = {
	{ VAL_NIL,   "nil" },
	{ VAL_INT,   "integer" },
	{ VAL_FLOAT, "float" },
	{ VAL_STR,   "string" },
	{ VAL_BOOL,  "boolean" },
	{ VAL_LIST,  "list" }
};

#define BINARY_MATH_OPERATION(x)                                                    \
	if ((l.type == VAL_INT || l.type == VAL_FLOAT)                              \
	    && (r.type == VAL_INT || r.type == VAL_FLOAT)) {                        \
		if (l.type == VAL_FLOAT || r.type == VAL_FLOAT) {                   \
			ret.type = VAL_FLOAT;                                       \
			ret.real = (double)(l.type == VAL_INT ? l.integer : l.real) \
				x (double)(r.type == VAL_INT ? r.integer : r.real); \
		} else {                                                            \
			ret.type = VAL_INT;                                         \
			ret.integer = l.integer x r.integer;                        \
		}                                                                   \
	}

#define INVALID_BINARY_OPERATION \
	error_push(vm->r, vm->code[vm->ip].loc, ERR_FATAL, "invalid operation on types '%s' and '%s'", value_data[l.type].body, value_data[r.type].body)

#define INVALID_UNARY_OPERATION \
	error_push(vm->r, vm->code[vm->ip].loc, ERR_FATAL, "invalid operation on type '%s'", value_data[l.type].body)

#define NONEXISTANT_VALUE_ERROR(x)                                \
	DOUT("internal error; found a value of type %d", x.type); \
	assert(false)

struct value
add_values(struct vm *vm, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_NIL;

	if (l.type == VAL_STR && r.type == VAL_STR) {
		ret.type = VAL_STR;

		ret.str.text = oak_malloc(strlen(l.str.text) + strlen(r.str.text) + 1);

		strcpy(ret.str.text, l.str.text);
		strcpy(ret.str.text + strlen(l.str.text), r.str.text);

		ret.str.len = strlen(ret.str.text);
	} else BINARY_MATH_OPERATION(+) else {
		INVALID_BINARY_OPERATION;
		vm_panic(vm);
	}

	return ret;
}

struct value sub_values(struct vm *vm, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(-) else {
		INVALID_BINARY_OPERATION;
		vm_panic(vm);
	}

	return ret;
}

struct value mul_values(struct vm *vm, struct value l, struct value r)
{
	/* TODO: fancy operations on strings and lists. */
	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(*) else {
		INVALID_BINARY_OPERATION;
		vm_panic(vm);
	}

	return ret;
}

struct value div_values(struct vm *vm, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(/) else {
		INVALID_BINARY_OPERATION;
		vm_panic(vm);
	}

	return ret;
}

struct value
mod_values(struct vm *vm, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_NIL;

	if ((l.type == VAL_INT || l.type == VAL_FLOAT)
	    && (r.type == VAL_INT || r.type == VAL_FLOAT)) {
		if (l.type == VAL_FLOAT || r.type == VAL_FLOAT) {
			ret.type = VAL_FLOAT;
			ret.real = fmod((double)(l.type == VAL_INT ? l.integer : l.real), (double)(r.type == VAL_INT ? r.integer : r.real));
		} else {
			ret.type = VAL_INT;
			ret.integer = l.integer % r.integer;
		}
	} else {
		INVALID_BINARY_OPERATION;
		vm_panic(vm);
	}

	return ret;
}

struct value
is_less_than_value(struct vm *vm, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(<) else {
		INVALID_BINARY_OPERATION;
		vm_panic(vm);
	}

	return ret;
}

struct value
cmp_values(struct vm *vm, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_NIL;

	if (l.type != r.type) INVALID_BINARY_OPERATION;

	ret.type = VAL_BOOL;
	switch (l.type) {
	case VAL_BOOL:  ret.boolean = (l.boolean == r.boolean);        break;
	case VAL_INT:   ret.boolean = (l.integer == r.integer);        break;
	case VAL_FLOAT: ret.boolean = (l.real == r.real);              break;
	case VAL_STR:   ret.boolean = !strcmp(l.str.text, r.str.text); break;
	case VAL_NIL:   ret.boolean = true;                            break;
	default:        NONEXISTANT_VALUE_ERROR(l);                    break;
	}

	return ret;
}

struct value
and_values(struct vm *vm, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_BOOL;

	if (l.type != VAL_BOOL || r.type != VAL_BOOL) INVALID_BINARY_OPERATION;
	ret.boolean = l.boolean && r.boolean;

	return ret;
}

bool
is_value_true(struct vm *vm, struct value l)
{
	if (l.type != VAL_BOOL) INVALID_UNARY_OPERATION;
	return l.boolean;
}

struct value
inc_value(struct vm *vm, struct value l)
{
	switch (l.type) {
	case VAL_INT:   l.integer++;      break;
	case VAL_FLOAT: l.real++;         break;
	default: INVALID_UNARY_OPERATION; break;
	}

	return l;
}

struct value
dec_value(struct vm *vm, struct value l)
{
	switch (l.type) {
	case VAL_INT:   l.integer--;      break;
	case VAL_FLOAT: l.real--;         break;
	default: INVALID_UNARY_OPERATION; break;
	}

	return l;
}

void
print_value(FILE *f, struct value val)
{
	switch (val.type) {
	case VAL_INT:
		fprintf(f, "%"PRId64, val.integer);
		break;
	case VAL_STR:
		fprintf(f, "%s", val.str.text);
		break;
	case VAL_FLOAT:
		fprintf(f, "%f", val.real);
		break;
	case VAL_LIST:
		fputc('[', f);

		for (uint64_t i = val.list->len - 1; i + 1; i--) {
			print_value(f, val.list->val[i]);
			if (i + 1 != 1)
				fprintf(f, ", ");
		}

		fputc(']', f);
		break;
	case VAL_NIL:
		fprintf(f, "nil");
		break;
	default:
		DOUT("unimplemented printer for value of type %d", val.type);
		assert(false);
	}
}
