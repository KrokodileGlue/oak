#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>

#include "util.h"
#include "value.h"

struct value_data value_data[] = {
	{ VAL_STR,   "string" },
	{ VAL_ARRAY, "array" },
	{ VAL_NIL,   "nil" },
	{ VAL_INT,   "integer" },
	{ VAL_FLOAT, "float" },
	{ VAL_BOOL,  "boolean" },
	{ VAL_FN,    "function" }
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

static char *
show_value(struct gc *gc, struct value val)
{
	// TODO: remove the artificial cap.
	size_t cap = 2048;
	char *str = oak_malloc(cap + 1);
	*str = 0;

	switch (val.type) {
	case VAL_INT:
		snprintf(str, cap, "%"PRId64, val.integer);
		break;

	case VAL_STR:
		snprintf(str, cap, "%s", gc->str[val.idx]);
		break;

	case VAL_FLOAT:
		snprintf(str, cap, "%f", val.real);
		break;

	case VAL_BOOL:
		snprintf(str, cap, "%s", val.boolean ? "true" : "false");
		break;

	case VAL_NIL:
		snprintf(str, cap, "nil");
		break;

	case VAL_ARRAY:
		for (unsigned int i = 0; i < gc->arrlen[val.idx]; i++) {
			char *asdf = show_value(gc, gc->array[val.idx][i]);
			char *temp = new_cat(str, asdf);
			free(str);
			str = temp;
			free(asdf);
		}
		break;

	default:
		DOUT("unimplemented printer for value of type %d", val.type);
		assert(false);
	}

	return str;
}

static void
print_debug(struct gc *gc, struct value l)
{
	fputc('\'', stderr);
	print_value(stderr, gc, l);
	fputc('\'', stderr);
	fprintf(stderr, " (%s)", value_data[l.type].body);
}

struct value
add_values(struct gc *gc, struct value l, struct value r)
{
	if (gc->debug) {
		fprintf(stderr, "addition - left: ");
		print_debug(gc, l);
		fprintf(stderr, ", right: ");
		print_debug(gc, r);
		fputc('\n', stderr);
	}

	struct value ret;
	ret.type = VAL_NIL;

	if (l.type == VAL_STR && r.type == VAL_STR) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		gc->str[ret.idx] = NULL;

		/*
		 * We should be careful about messing with memory
		 * that's owned by the garbage collector.
		 */

		gc->str[ret.idx] = oak_realloc(gc->str[ret.idx], strlen(gc->str[l.idx]) + strlen(gc->str[r.idx]) + 1);

		strcpy(gc->str[ret.idx], gc->str[l.idx]);
		strcpy(gc->str[ret.idx] + strlen(gc->str[l.idx]), gc->str[r.idx]);
	} else if (l.type == VAL_STR && r.type == VAL_INT) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		char *s = show_value(gc, r);
		gc->str[ret.idx] = new_cat(gc->str[l.idx], s);
		free(s);
	} else if (r.type == VAL_STR && l.type == VAL_INT) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		char *s = show_value(gc, l);
		gc->str[ret.idx] = new_cat(s, gc->str[r.idx]);
		free(s);
	} else if (l.type == VAL_STR && r.type == VAL_BOOL) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		char *s = show_value(gc, r);
		gc->str[ret.idx] = new_cat(gc->str[l.idx], s);
		free(s);
	} else if (r.type == VAL_STR && l.type == VAL_BOOL) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		char *s = show_value(gc, l);
		gc->str[ret.idx] = new_cat(gc->str[r.idx], s);
		free(s);
	} else if (l.type == VAL_STR && r.type == VAL_ARRAY) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		char *s = show_value(gc, r);
		gc->str[ret.idx] = new_cat(gc->str[l.idx], s);
		free(s);
	} else if (l.type == VAL_ARRAY && r.type == VAL_STR) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		char *s = show_value(gc, l);
		gc->str[ret.idx] = new_cat(s, gc->str[r.idx]);
		free(s);
	} else if (l.type == VAL_STR && r.type == VAL_NIL) {
		ret = copy_value(gc, l);
	} else if (l.type == VAL_NIL && r.type == VAL_STR) {
		ret = copy_value(gc, r);
	} else if (l.type == VAL_INT && r.type == VAL_NIL) {
		ret = copy_value(gc, l);
	} else if (l.type == VAL_NIL && r.type == VAL_INT) {
		ret = copy_value(gc, r);
	} else BINARY_MATH_OPERATION(+) else {
		/* TODO: do something here. */
		assert(false);
	}

	return ret;
}

struct value
sub_values(struct gc *gc, struct value l, struct value r)
{
	if (gc->debug) {
		fprintf(stderr, "subtraction - left: ");
		print_debug(gc, l);
		fprintf(stderr, ", right: ");
		print_debug(gc, r);
		fputc('\n', stderr);
	}

	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(-) else {
		assert(false);
	}

	return ret;
}

struct value
mul_values(struct gc *gc, struct value l, struct value r)
{
	if (gc->debug) {
		fprintf(stderr, "multiplication - left: ");
		print_debug(gc, l);
		fprintf(stderr, ", right: ");
		print_debug(gc, r);
		fputc('\n', stderr);
	}

	/* TODO: fancy operations on strings and lists. */
	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(*) else {
		assert(false);
	}

	return ret;
}

struct value
div_values(struct gc *gc, struct value l, struct value r)
{
	if (gc->debug) {
		fprintf(stderr, "division - left: ");
		print_debug(gc, l);
		fprintf(stderr, ", right: ");
		print_debug(gc, r);
		fputc('\n', stderr);
	}

	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(/) else {
		assert(false);
	}

	return ret;
}

struct value
mod_values(struct gc *gc, struct value l, struct value r)
{
	if (gc->debug) {
		fprintf(stderr, "modulus - left: ");
		print_debug(gc, l);
		fprintf(stderr, ", right: ");
		print_debug(gc, r);
		fputc('\n', stderr);
	}

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
		assert(false);
	}

	return ret;
}

struct value
value_less(struct gc *gc, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(<) else {
		assert(false);
	}

	return ret;
}

struct value
is_more_than_value(struct gc *gc, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_NIL;

	BINARY_MATH_OPERATION(>) else {
		assert(false);
	}

	return ret;
}

struct value
cmp_values(struct gc *gc, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_BOOL;

	if (l.type != r.type) {
		ret.boolean = false;
		return ret;
	}

	switch (l.type) {
	case VAL_BOOL:  ret.boolean = (l.boolean == r.boolean);                break;
	case VAL_INT:   ret.boolean = (l.integer == r.integer);                break;
	case VAL_FLOAT: ret.boolean = (l.real == r.real);                      break;
	case VAL_STR:   ret.boolean = !strcmp(gc->str[l.idx], gc->str[r.idx]); break;
	case VAL_NIL:   ret.boolean = true;                                    break;
	default:        assert(false);                                         break;
	}

	return ret;
}

struct value
and_values(struct gc *gc, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_BOOL;

	if (l.type != VAL_BOOL || r.type != VAL_BOOL) {
		assert(false);
	}

	ret.boolean = l.boolean && r.boolean;

	return ret;
}

struct value
or_values(struct gc *gc, struct value l, struct value r)
{
	struct value ret;
	ret.type = VAL_BOOL;

	if (l.type != VAL_BOOL || r.type != VAL_BOOL) {
		assert(false);
	}

	ret.boolean = l.boolean || r.boolean;

	return ret;
}

bool
is_truthy(struct gc *gc, struct value l)
{
	/* TODO: Flesh this stuff out. */
	switch (l.type) {
	case VAL_BOOL: return l.boolean;
	case VAL_INT: return l.integer != 0;
	default:
		return false;
	}
}

struct value
inc_value(struct gc *gc, struct value l)
{
	switch (l.type) {
	case VAL_INT:   l.integer++;      break;
	case VAL_FLOAT: l.real++;         break;
	default: assert(false); break;
	}

	return l;
}

struct value
dec_value(struct gc *gc, struct value l)
{
	switch (l.type) {
	case VAL_INT:   l.integer--; break;
	case VAL_FLOAT: l.real--;    break;
	default: assert(false); break;
	}

	return l;
}

struct value
value_len(struct gc *gc, struct value l)
{
	struct value ans;
	ans.type = VAL_INT;

	switch (l.type) {
	case VAL_STR:  ans.integer = strlen(gc->str[l.idx]); break;
	case VAL_ARRAY: ans.integer = gc->arrlen[l.idx]; break;
	default: assert(false); break;
	}

	return ans;
}

struct value
copy_value(struct gc *gc, struct value l)
{
	if (l.type == VAL_ARRAY) {
		struct value v;
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(gc, VAL_ARRAY);
		gc->arrlen[v.idx] = 0;
		gc->array[v.idx] = NULL;

		for (size_t i = 0; i < gc->arrlen[l.idx]; i++) {
			pushback(gc, v, copy_value(gc, gc->array[l.idx][i]));
		}

		l = v;
	}

	return l;
}

struct value
flip_value(struct gc *gc, struct value l)
{
	struct value ans;
	ans.type = VAL_BOOL;

	switch (l.type) {
	case VAL_INT:   ans.boolean = l.integer ? false : true; break;
	case VAL_FLOAT: ans.boolean = l.real    ? false : true; break;
	case VAL_BOOL:  ans.boolean = !l.boolean;               break;
	default: assert(false); break;
	}

	return ans;
}

struct value
neg_value(struct gc *gc, struct value l)
{
	struct value ans;
	ans.type = l.type;

	switch (l.type) {
	case VAL_INT:   ans.integer = -l.integer; break;
	case VAL_FLOAT: ans.real = -l.real;       break;
	case VAL_BOOL:  ans.boolean = !l.boolean; break;
	default: assert(false);         break;
	}

	return ans;
}

struct value
pushback(struct gc *gc, struct value l, struct value r)
{
	if (gc->debug) {
		fprintf(stderr, "pushback - left: ");
		print_debug(gc, l);
		fprintf(stderr, ", right: ");
		print_debug(gc, r);
		fputc('\n', stderr);
	}

	/* TODO: Make sure l is an array. */
	assert(l.type == VAL_ARRAY);
	gc->array[l.idx] = oak_realloc(gc->array[l.idx], (gc->arrlen[l.idx] + 1) * sizeof **gc->array);
	gc->array[l.idx][gc->arrlen[l.idx]] = r;
	gc->arrlen[l.idx]++;
	return l;
}

struct value
grow_array(struct gc *gc, struct value l, int r)
{
	/* TODO: Unretard this. */
	if ((int)gc->arrlen[l.idx] <= r) {
		gc->array[l.idx] = oak_realloc(gc->array[l.idx],
		     (r + 1) * sizeof *gc->array[l.idx]);
		for (int i = gc->arrlen[l.idx]; i < r; i++) {
			gc->array[l.idx][i].type = VAL_NIL;
		}
		gc->arrlen[l.idx] = r;
	}

	return l;
}

void
print_value(FILE *f, struct gc *gc, struct value val)
{
	switch (val.type) {
	case VAL_INT:
		fprintf(f, "%"PRId64, val.integer);
		break;

	case VAL_STR:
		fprintf(f, "%s", gc->str[val.idx]);
		break;

	case VAL_FLOAT:
		fprintf(f, "%.*g", DECIMAL_DIG - 5, val.real);
		break;

	case VAL_BOOL:
		fprintf(f, val.boolean ? "true" : "false");
		break;

	case VAL_NIL:
		break;

	case VAL_ARRAY:
		for (unsigned int i = 0; i < gc->arrlen[val.idx]; i++)
			print_value(f, gc, gc->array[val.idx][i]);
		break;

	case VAL_FN:
		fprintf(f, "%"PRId64, val.integer);
		break;

	default:
		DOUT("unimplemented printer for value of type %d", val.type);
		assert(false);
	}
}
