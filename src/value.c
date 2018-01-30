#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>

#include "util.h"
#include "value.h"

struct value_data value_data[] = {
	{ VAL_STR,   "string"   },
	{ VAL_ARRAY, "array"    },
	{ VAL_REGEX, "regex"    },
	{ VAL_TABLE, "table"    },
	{ VAL_NIL,   "nil"      },
	{ VAL_INT,   "integer"  },
	{ VAL_FLOAT, "float"    },
	{ VAL_BOOL,  "boolean"  },
	{ VAL_FN,    "function" },
	{ VAL_UNDEF, "undef"    },
	{ VAL_ERR,   "error"    }
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

char *
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
		*str = 0;
		break;

	case VAL_ARRAY:
		for (unsigned int i = 0; i < gc->array[val.idx]->len; i++) {
			char *asdf = show_value(gc, gc->array[val.idx]->v[i]);
			char *temp = new_cat(str, asdf);
			free(str);
			str = temp;
			free(asdf);
		}
		break;

	case VAL_REGEX:
		snprintf(str, cap, "REGEX(%p)", (void *)gc->regex[val.idx]);
		break;

	default:
		DOUT("unimplemented printer for value of type %d", val.type);
		assert(false);
	}

	return str;
}

void
print_debug(struct gc *gc, struct value l)
{
	fputc('\'', stderr);
	print_value(stderr, gc, l);
	fputc('\'', stderr);
	fprintf(stderr, " (%s)", value_data[l.type].body);
}

static void
_log(struct gc *gc, const char *msg, struct value l, struct value r)
{
	if (gc->debug) {
		fprintf(stderr, "%s - left: ", msg);
		print_debug(gc, l);
		fprintf(stderr, ", right: ");
		print_debug(gc, r);
		fputc('\n', stderr);
	}
}

struct value
add_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "addition", l, r);

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
		return ERR("Invalid operation");
	}

	return ret;
}

struct value
sub_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "subtraction", l, r);

	struct value ret;
	ret.type = VAL_NIL;

	if (l.type == VAL_NIL || r.type == VAL_NIL) return NIL;

	BINARY_MATH_OPERATION(-) else {
		assert(false);
	}

	return ret;
}

struct value
mul_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "multiplication", l, r);

	struct value ret;
	ret.type = VAL_NIL;

	if (l.type == VAL_NIL || r.type == VAL_NIL)
		return ret;

	if (l.type == VAL_STR && r.type == VAL_INT) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		gc->str[ret.idx] = oak_malloc(r.integer * strlen(gc->str[l.idx]) + 1);
		*gc->str[ret.idx] = 0;

		for (int64_t i = 0; i < r.integer; i++)
			strcat(gc->str[ret.idx], gc->str[l.idx]);
	} else if (l.type == VAL_INT && r.type == VAL_STR) {
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		gc->str[ret.idx] = oak_malloc(l.integer * strlen(gc->str[r.idx]) + 1);
		*gc->str[ret.idx] = 0;

		for (int64_t i = 0; i < l.integer; i++)
			strcat(gc->str[ret.idx], gc->str[r.idx]);
	} else if (l.type == VAL_ARRAY && r.type == VAL_INT) {
		ret.type = VAL_ARRAY;
		ret.idx = gc_alloc(gc, VAL_ARRAY);
		gc->array[ret.idx] = new_array();

		for (int64_t i = 0; i < r.integer; i++)
			for (size_t j = 0; j < gc->array[l.idx]->len; j++)
				array_push(gc->array[ret.idx], copy_value(gc, gc->array[l.idx]->v[j]));
	} else if (l.type == VAL_INT && r.type == VAL_ARRAY) {
		ret.type = VAL_ARRAY;
		ret.idx = gc_alloc(gc, VAL_ARRAY);
		gc->array[ret.idx] = new_array();

		for (int64_t i = 0; i < l.integer; i++)
			for (size_t j = 0; j < gc->array[r.idx]->len; j++)
				array_push(gc->array[ret.idx], copy_value(gc, gc->array[r.idx]->v[j]));
	} else BINARY_MATH_OPERATION(*) else {
		assert(false);
	}

	return ret;
}

struct value
pow_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "power", l, r);

	if (l.type == VAL_INT || r.type == VAL_INT) {
		if (r.integer == 0) {
			l.integer = 1;
			return l;
		}

		int64_t base = l.integer;
		for (int i = 0; i < r.integer - 1; i++)
			l.integer *= base;
	} else {
		assert(false);
	}

	return l;
}

struct value
div_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "division", l, r);

	struct value ret;
	ret.type = VAL_NIL;

	if ((r.type == VAL_INT && r.integer == 0) || (r.type == VAL_FLOAT && fcmp(r.real, 0))) {
		return ERR("division by zero");
	}

	BINARY_MATH_OPERATION(/) else {
		assert(false);
	}

	return ret;
}

struct value
mod_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "modulus", l, r);

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
less_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "lessthan", l, r);

	struct value ret;
	ret.type = VAL_NIL;

	if (l.type == VAL_NIL || r.type == VAL_NIL) return ret;

	BINARY_MATH_OPERATION(<) else {
		return ERR("invalid operation");
	}

	if (ret.type == VAL_INT && ret.integer == 0) {
		ret.type = VAL_BOOL;
		ret.boolean = false;
	} else if (ret.type == VAL_FLOAT && fcmp(ret.real, 0)) {
		ret.type = VAL_BOOL;
		ret.boolean = false;
	} else {
		ret.type = VAL_BOOL;
		ret.boolean = true;
	}

	return ret;
}

struct value
leq_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "leq", l, r);

	struct value ret;
	ret.type = VAL_BOOL;

	BINARY_MATH_OPERATION(<=) else {
		assert(false);
	}

	ret.boolean = ret.integer == 0;
	return ret;
}

struct value
geq_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "geq", l, r);

	struct value ret;
	ret.type = VAL_BOOL;

	if (l.type == VAL_NIL || r.type == VAL_NIL) return NIL;

	BINARY_MATH_OPERATION(>=) else {
		return ERR("invalid operation");
	}

	ret.boolean = ret.integer == 0;
	return ret;
}

struct value
more_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "morethan", l, r);

	struct value ret;
	ret.type = VAL_NIL;

	if (l.type == VAL_NIL || r.type == VAL_NIL) return NIL;

	BINARY_MATH_OPERATION(>) else {
		return BOOL(false);
	}

	struct value ret2;
	ret2.type = VAL_BOOL;
	ret2.boolean = is_truthy(gc, ret);

	return ret2;
}

struct value
cmp_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "cmp", l, r);

	struct value ret;
	ret.type = VAL_BOOL;

	if (l.type != r.type) {
		ret.boolean = false;
		return ret;
	}

	switch (l.type) {
	case VAL_BOOL:  ret.boolean = (l.boolean == r.boolean);                break;
	case VAL_INT:   ret.boolean = (l.integer == r.integer);                break;
	case VAL_FLOAT: ret.boolean = fcmp(l.real, r.real);                    break;
	case VAL_STR:   ret.boolean = !strcmp(gc->str[l.idx], gc->str[r.idx]); break;
	case VAL_NIL:   ret.boolean = (r.type == VAL_NIL);                     break;
	case VAL_ARRAY:
		ret.boolean = false;

		if (r.type != VAL_ARRAY || gc->array[l.idx]->len != gc->array[r.idx]->len)
			return ret;

		for (size_t i = 0; i < gc->array[l.idx]->len; i++)
			if (!cmp_values(gc, gc->array[l.idx]->v[i], gc->array[r.idx]->v[i]).boolean)
				return ret;

		ret.boolean = true;
		break;

	default:
		assert(false);
		break;
	}

	return ret;
}

struct value
and_values(struct gc *gc, struct value l, struct value r)
{
	_log(gc, "and", l, r);

	struct value ret;
	ret.type = VAL_BOOL;

	if (l.type != VAL_BOOL || r.type != VAL_BOOL) {
		assert(false);
	}

	ret.boolean = l.boolean && r.boolean;

	return ret;
}

bool
is_truthy(struct gc *gc, struct value l)
{
	_log(gc, "truthy", l, (struct value){ VAL_NIL, {0}, 0 });

	switch (l.type) {
	case VAL_BOOL:  return l.boolean;
	case VAL_INT:   return l.integer != 0;
	case VAL_STR:   return !!strlen(gc->str[l.idx]);
	case VAL_ARRAY: return !!gc->array[l.idx]->len;
	case VAL_FLOAT: return !fcmp(l.real, 0.0);
	case VAL_REGEX: return !!gc->regex[l.idx]->num_matches;
	case VAL_NIL:   return false;
	case VAL_FN:    return true;
	case VAL_ERR:   assert(false);
	case VAL_UNDEF: assert(false);
	case VAL_TABLE: {
		int len = 0;
		for (int i = 0; i < TABLE_SIZE; i++)
			len += gc->table[l.idx]->bucket[i].len;
		return !!len;
	} break;
	}

	return false;
}

struct value
inc_value(struct value l)
{
	switch (l.type) {
	case VAL_INT:   l.integer++;      break;
	case VAL_FLOAT: l.real++;         break;
	default: return (struct value){ VAL_ERR, { 0 }, 0 };
	}

	return l;
}

struct value
dec_value(struct value l)
{
	switch (l.type) {
	case VAL_INT:   l.integer--; break;
	case VAL_FLOAT: l.real--;    break;
	default: return (struct value){ VAL_ERR, { 0 }, 0 };
	}

	return l;
}

struct value
value_len(struct gc *gc, struct value l)
{
	_log(gc, "length", l, (struct value){ VAL_NIL, {0}, 0 });

	struct value ans;
	ans.type = VAL_INT;

	switch (l.type) {
	case VAL_STR:   ans.integer = strlen(gc->str[l.idx]); break;
	case VAL_ARRAY: ans.integer = gc->array[l.idx]->len;  break;
	case VAL_NIL:   ans.integer = 0;                      break;
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
		gc->array[v.idx] = new_array();

		for (size_t i = 0; i < gc->array[l.idx]->len; i++)
			array_push(gc->array[v.idx], copy_value(gc, gc->array[l.idx]->v[i]));

		l = v;
	} else if (l.type == VAL_STR) {
		struct value v;
		v.type = VAL_STR;
		v.idx = gc_alloc(gc, VAL_STR);
		gc->str[v.idx] = strclone(gc->str[l.idx]);
		l = v;
	}

	return l;
}

struct value
flip_value(struct gc *gc, struct value l)
{
	_log(gc, "flip", l, (struct value){ VAL_NIL, {0}, 0 });
	struct value ans = (struct value){ VAL_BOOL, { 0 }, 0 };

	switch (l.type) {
	case VAL_INT:   ans.boolean = !l.integer;              break;
	case VAL_FLOAT: ans.boolean = !fcmp(l.real, 0.0);      break;
	case VAL_BOOL:  ans.boolean = !l.boolean;              break;
	case VAL_NIL:   ans.boolean = true;                    break;
	case VAL_ARRAY: ans.boolean = !gc->array[l.idx]->len;  break;
	case VAL_STR:   ans.boolean = !strlen(gc->str[l.idx]); break;
	default: assert(false);
	}

	return ans;
}

static void
qsort_partition(struct gc *gc, struct value l, int begin, int end)
{
	if (begin < 0 || end < 0) return;
	if (begin >= end) return;

	int pivot = begin;
	int i = begin + 1;
	int j = begin + 1;

	for (i = begin + 1; i <= end; i++) {
		if (less_values(gc, gc->array[l.idx]->v[i], gc->array[l.idx]->v[pivot]).boolean) {
			struct value t = gc->array[l.idx]->v[i];
			gc->array[l.idx]->v[i] = gc->array[l.idx]->v[j];
			gc->array[l.idx]->v[j] = t;
			j++;
		}
	}

	struct value t = gc->array[l.idx]->v[j - 1];
	gc->array[l.idx]->v[j - 1] = gc->array[l.idx]->v[pivot];
	gc->array[l.idx]->v[pivot] = t;

	qsort_partition(gc, l, begin, j - 2);
	qsort_partition(gc, l, j, end);
}

struct value
sort_value(struct gc *gc, struct value l)
{
	_log(gc, "sort", l, (struct value){ VAL_NIL, {0}, 0 });
	struct value ret;

	switch (l.type) {
	case VAL_STR:
		ret.type = VAL_STR;
		ret.idx = gc_alloc(gc, VAL_STR);
		gc->str[ret.idx] = strsort(gc->str[l.idx]);
		break;

	case VAL_ARRAY:
		ret = copy_value(gc, l);
		qsort_partition(gc, ret, 0, (int)gc->array[l.idx]->len - 1);
		break;

	case VAL_NIL:
		return l;

	default: assert(false);
	}

	return ret;
}

struct value
max_value(struct gc *gc, struct value l)
{
	struct value v;

	if (l.type == VAL_TABLE) {
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(gc, VAL_ARRAY);
		gc->array[v.idx] = new_array();

		for (int i = 0; i < TABLE_SIZE; i++) {
			struct bucket b = gc->table[l.idx]->bucket[i];
			for (size_t j = 0; j < b.len; j++) {
				array_push(gc->array[v.idx], b.val[j]);
			}
		}
	} else {
		if (l.type != VAL_ARRAY)
			return ERR("max expects an array or table");
		v = l;
	}

	if (gc->array[v.idx]->len == 0) return NIL;
	struct value m = gc->array[v.idx]->v[0];

	for (unsigned i = 0; i < gc->array[v.idx]->len; i++)
		if (is_truthy(gc, more_values(gc, gc->array[v.idx]->v[i], m)))
			m = gc->array[v.idx]->v[i];

	return m;
}

struct value
rev_value(struct gc *gc, struct value l)
{
	struct value ans = (struct value){ VAL_ERR, { 0 }, 0 };

	switch (l.type) {
	case VAL_STR: {
		ans.type = VAL_STR;
		ans.idx = gc_alloc(gc, VAL_STR);

		size_t len = strlen(gc->str[l.idx]);
		gc->str[ans.idx] = oak_malloc(len + 1);

		for (size_t i = 1; i <= len; i++)
			gc->str[ans.idx][i - 1] = gc->str[l.idx][len - i];

		gc->str[ans.idx][len] = 0;
	} break;

	case VAL_ARRAY: {
		ans.type = VAL_ARRAY;
		ans.idx = gc_alloc(gc, VAL_ARRAY);

		gc->array[ans.idx] = new_array();
		size_t len = gc->array[l.idx]->len;

		for (size_t i = 1; i <= len; i++) {
			struct value r = gc->array[l.idx]->v[len - i];
			array_push(gc->array[ans.idx], copy_value(gc, r));
		}
	} break;

	default: assert(false);
	}

	return ans;
}

struct value
abs_value(struct value l)
{
	struct value ret;
	ret.type = VAL_NIL;

	switch (l.type) {
	case VAL_INT:
		ret.type = VAL_INT;
		ret.integer = l.integer < 0 ? -l.integer : l.integer;
		break;

	case VAL_FLOAT:
		ret.type = VAL_FLOAT;
		ret.real = l.real < 0 ? -l.real : l.real;
		break;

	default:
		assert(false);
	}

	return ret;
}

struct value
uc_value(struct gc *gc, struct value l)
{
	struct value ans = (struct value){ VAL_ERR, { 0 }, 0 };

	if (l.type == VAL_STR) {
		ans.type = VAL_STR;
		ans.idx = gc_alloc(gc, VAL_STR);
		size_t len = strlen(gc->str[l.idx]);
		gc->str[ans.idx] = oak_malloc(len + 1);

		for (size_t i = 0; i <= len; i++)
			gc->str[ans.idx][i] = uc(gc->str[l.idx][i]);
	}

	return ans;
}

struct value
lc_value(struct gc *gc, struct value l)
{
	struct value ans = (struct value){ VAL_ERR, { 0 }, 0 };

	if (l.type == VAL_STR) {
		ans.type = VAL_STR;
		ans.idx = gc_alloc(gc, VAL_STR);
		size_t len = strlen(gc->str[l.idx]);
		gc->str[ans.idx] = oak_malloc(len + 1);

		for (size_t i = 0; i <= len; i++)
			gc->str[ans.idx][i] = lc(gc->str[l.idx][i]);
	}

	return ans;
}

struct value
ucfirst_value(struct gc *gc, struct value l)
{
	struct value ans = (struct value){ VAL_ERR, { 0 }, 0 };

	if (l.type == VAL_STR) {
		ans.type = VAL_STR;
		ans.idx = gc_alloc(gc, VAL_STR);

		size_t len = strlen(gc->str[l.idx]);

		gc->str[ans.idx] = oak_malloc(len + 1);
		strcpy(gc->str[ans.idx], gc->str[l.idx]);
		gc->str[ans.idx][0] = uc(gc->str[ans.idx][0]);
	}

	return ans;
}

struct value
lcfirst_value(struct gc *gc, struct value l)
{
	struct value ans = (struct value){ VAL_ERR, { 0 }, 0 };

	if (l.type == VAL_STR) {
		ans.type = VAL_STR;
		ans.idx = gc_alloc(gc, VAL_STR);

		size_t len = strlen(gc->str[l.idx]);

		gc->str[ans.idx] = oak_malloc(len + 1);
		strcpy(gc->str[ans.idx], gc->str[l.idx]);
		gc->str[ans.idx][0] = lc(gc->str[ans.idx][0]);
	}

	return ans;
}

struct value
neg_value(struct value l)
{
	struct value ans = (struct value){ l.type, { 0 }, 0 };

	switch (l.type) {
	case VAL_INT:   ans.integer = -l.integer; break;
	case VAL_FLOAT: ans.real = -l.real;       break;
	case VAL_BOOL:  ans.boolean = !l.boolean; break;
	default: return (struct value){ VAL_ERR, { 0 }, 0 };
	}

	return ans;
}

struct value
int_value(struct gc *gc, struct value l)
{
	struct value ret;
	ret.type = VAL_INT;
	char *end = NULL;
	ret.integer = (int64_t)strtol(gc->str[l.idx], &end, 0);

	/* TODO: Maybe the gc should have a reporter. */
	/* if (*end) { */
	/* } */

	return ret;
}

struct value
float_value(struct gc *gc, struct value l)
{
	struct value ret;
	ret.type = VAL_FLOAT;
	char *end = NULL;
	ret.real = strtod(gc->str[l.idx], &end);
	return ret;
}

struct value
value_translate(struct gc *l, struct gc *r, struct value v)
{
	if (l->debug || r->debug) {
		fprintf(stderr, "translating ");
		print_debug(r, v);
		fprintf(stderr, " from %p to %p\n", (void *)r, (void *)l);
	}

	if (l == r) return v;

	if (v.type < NUM_ALLOCATABLE_VALUES) {
		struct value ret;
		ret.type = v.type;
		ret.idx = gc_alloc(l, v.type);

		switch (v.type) {
		case VAL_STR:
			l->str[ret.idx] = strclone(r->str[v.idx]);
			break;

		case VAL_ARRAY:
			l->array[ret.idx] = new_array();
			for (size_t i = 0; i < r->array[v.idx]->len; i++)
				array_push(l->array[ret.idx], value_translate(l, r, r->array[v.idx]->v[i]));
			break;

		case VAL_REGEX:
			l->regex[ret.idx] = ktre_copy(r->regex[v.idx]);
			break;

		default: assert(false);
		}

		return ret;
	}

	return v;
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
		for (size_t i = 0; i < gc->array[val.idx]->len; i++)
			print_value(f, gc, gc->array[val.idx]->v[i]);
		break;

	case VAL_FN:
		fprintf(f, "FUNCTION(%"PRId64")", val.integer);
		break;

	case VAL_REGEX:
		fprintf(f, "REGEX(%p)", (void *)gc->regex[val.idx]);
		break;

	case VAL_TABLE:
		for (int i = 0; i < TABLE_SIZE; i++) {
			for (size_t j = 0; j < gc->table[val.idx]->bucket[i].len; j++) {
				print_value(f, gc, gc->table[val.idx]->bucket[i].val[j]);
			}
		}
		break;

	default:
		DOUT("unimplemented printer for value of type %d", val.type);
		assert(false);
	}
}
