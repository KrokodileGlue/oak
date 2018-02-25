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

#define BINARY_MATH_OPERATION(X,Y)	  \
	if ((l.type == VAL_INT || l.type == VAL_FLOAT) \
	    && (r.type == VAL_INT || r.type == VAL_FLOAT)) { \
		if (l.type == VAL_FLOAT || r.type == VAL_FLOAT) { \
			(X).type = VAL_FLOAT; \
			(X).real = (double)(l.type == VAL_INT ? l.integer : l.real) \
				Y (double)(r.type == VAL_INT ? r.integer : r.real); \
		} else { \
			(X).type = VAL_INT; \
			(X).integer = l.integer Y r.integer; \
		} \
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

struct value
val_binop(struct gc *gc, struct value l, struct value r, int op)
{
	struct value v;
	v.type = VAL_NIL;

	if (gc->debug) {
		fprintf(stderr, "binary operation %d on ", op);
		print_debug(gc, l);
		fprintf(stderr, " and ");
		print_debug(gc, r);
		fprintf(stderr, "\n");
	}

	switch (op) {
	case OP_ADD:
		if (l.type == VAL_STR && r.type == VAL_STR) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			gc->str[v.idx] = NULL;

			gc->str[v.idx] = oak_realloc(gc->str[v.idx], strlen(gc->str[l.idx]) + strlen(gc->str[r.idx]) + 1);

			strcpy(gc->str[v.idx], gc->str[l.idx]);
			strcpy(gc->str[v.idx] + strlen(gc->str[l.idx]), gc->str[r.idx]);
		} else if (l.type == VAL_STR && r.type == VAL_INT) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			char *s = show_value(gc, r);
			gc->str[v.idx] = new_cat(gc->str[l.idx], s);
			free(s);
		} else if (r.type == VAL_STR && l.type == VAL_INT) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			char *s = show_value(gc, l);
			gc->str[v.idx] = new_cat(s, gc->str[r.idx]);
			free(s);
		} else if (l.type == VAL_STR && r.type == VAL_BOOL) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			char *s = show_value(gc, r);
			gc->str[v.idx] = new_cat(gc->str[l.idx], s);
			free(s);
		} else if (r.type == VAL_STR && l.type == VAL_BOOL) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			char *s = show_value(gc, l);
			gc->str[v.idx] = new_cat(gc->str[r.idx], s);
			free(s);
		} else if (l.type == VAL_STR && r.type == VAL_ARRAY) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			char *s = show_value(gc, r);
			gc->str[v.idx] = new_cat(gc->str[l.idx], s);
			free(s);
		} else if (l.type == VAL_ARRAY && r.type == VAL_STR) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			char *s = show_value(gc, l);
			gc->str[v.idx] = new_cat(s, gc->str[r.idx]);
			free(s);
		} else if (l.type == VAL_ARRAY && r.type == VAL_ARRAY) {
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(gc, VAL_ARRAY);
			gc->array[v.idx] = new_array();

			for (size_t i = 0; i < gc->array[l.idx]->len; i++)
				array_push(gc->array[v.idx], gc->array[l.idx]->v[i]);

			for (size_t i = 0; i < gc->array[r.idx]->len; i++)
				array_push(gc->array[v.idx], gc->array[r.idx]->v[i]);
		} else if (l.type == VAL_STR && r.type == VAL_NIL) {
			v = copy_value(gc, l);
		} else if (l.type == VAL_NIL && r.type == VAL_STR) {
			v = copy_value(gc, r);
		} else if (l.type == VAL_INT && r.type == VAL_NIL) {
			v = copy_value(gc, l);
		} else if (l.type == VAL_NIL && r.type == VAL_INT) {
			v = copy_value(gc, r);
		} else BINARY_MATH_OPERATION(v, +) else goto err;
		break;

	case OP_SUB:
		BINARY_MATH_OPERATION(v, -) else goto err;
		break;

	case OP_MUL:
		if (l.type == VAL_STR && r.type == VAL_INT) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			gc->str[v.idx] = oak_malloc(r.integer * strlen(gc->str[l.idx]) + 1);
			*gc->str[v.idx] = 0;

			for (int64_t i = 0; i < r.integer; i++)
				strcat(gc->str[v.idx], gc->str[l.idx]);
		} else if (l.type == VAL_INT && r.type == VAL_STR) {
			v.type = VAL_STR;
			v.idx = gc_alloc(gc, VAL_STR);
			gc->str[v.idx] = oak_malloc(l.integer * strlen(gc->str[r.idx]) + 1);
			*gc->str[v.idx] = 0;

			for (int64_t i = 0; i < l.integer; i++)
				strcat(gc->str[v.idx], gc->str[r.idx]);
		} else if (l.type == VAL_ARRAY && r.type == VAL_INT) {
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(gc, VAL_ARRAY);
			gc->array[v.idx] = new_array();

			for (int64_t i = 0; i < r.integer; i++)
				for (size_t j = 0; j < gc->array[l.idx]->len; j++)
					array_push(gc->array[v.idx], copy_value(gc, gc->array[l.idx]->v[j]));
		} else if (l.type == VAL_INT && r.type == VAL_ARRAY) {
			v.type = VAL_ARRAY;
			v.idx = gc_alloc(gc, VAL_ARRAY);
			gc->array[v.idx] = new_array();

			for (int64_t i = 0; i < l.integer; i++)
				for (size_t j = 0; j < gc->array[r.idx]->len; j++)
					array_push(gc->array[v.idx], copy_value(gc, gc->array[r.idx]->v[j]));
		} else BINARY_MATH_OPERATION(v, *) else goto err;
		break;

	case OP_POW:
		if (l.type == VAL_INT || r.type == VAL_INT) {
			if (r.integer == 0) {
				l.integer = 1;
				return l;
			}

			int64_t base = l.integer;
			for (int i = 0; i < r.integer - 1; i++)
				l.integer *= base;

			return l;
		} else goto err;

	case OP_DIV:
		if ((r.type == VAL_INT && r.integer == 0)
		    || (r.type == VAL_FLOAT && fcmp(r.real, 0))) {
			return ERR("division by zero");
		}

		BINARY_MATH_OPERATION(v, /) else goto err;
		break;

#define MATHOP(X,Y) \
	case OP_##X: \
		BINARY_MATH_OPERATION(v, Y) else goto err; \
		return BOOL(is_truthy(gc, v))

#define BITOP(X,Y) \
	case OP_##X: \
		if (l.type != VAL_INT && r.type != VAL_INT) goto err; \
		return INT(l.integer Y r.integer)

	MATHOP(MORE, >);
	MATHOP(LESS, <);
	MATHOP(LEQ, <=);
	MATHOP(GEQ, >=);
	MATHOP(NOTEQ, !=);
	BITOP(LEFT, <<);
	BITOP(RIGHT, >>);
	BITOP(BAND, &);
	BITOP(XOR, ^);
	BITOP(BOR, |);

	case OP_MOD:
		if ((l.type == VAL_INT || l.type == VAL_FLOAT)
		    && (r.type == VAL_INT || r.type == VAL_FLOAT)) {
			if (l.type == VAL_FLOAT || r.type == VAL_FLOAT) {
				v.type = VAL_FLOAT;
				v.real = fmod((double)(l.type == VAL_INT ? l.integer : l.real), (double)(r.type == VAL_INT ? r.integer : r.real));
			} else {
				v.type = VAL_INT;
				v.integer = l.integer % r.integer;
			}
		} else goto err;
		break;

	case OP_CMP:
		if (l.type != r.type) return BOOL(false);
		v.type = VAL_BOOL;

		switch (l.type) {
		case VAL_BOOL:  v.boolean = (l.boolean == r.boolean);                break;
		case VAL_INT:   v.boolean = (l.integer == r.integer);                break;
		case VAL_FLOAT: v.boolean = fcmp(l.real, r.real);                    break;
		case VAL_STR:   v.boolean = !strcmp(gc->str[l.idx], gc->str[r.idx]); break;
		case VAL_NIL:   v.boolean = (r.type == VAL_NIL);                     break;
		case VAL_ARRAY:
			v.boolean = false;

			if (r.type != VAL_ARRAY || gc->array[l.idx]->len != gc->array[r.idx]->len)
				return v;

			for (size_t i = 0; i < gc->array[l.idx]->len; i++)
				if (!is_truthy(gc, val_binop(gc, gc->array[l.idx]->v[i], gc->array[r.idx]->v[i], OP_CMP)))
					return v;

			v.boolean = true;
			break;

		default: assert(false);
		}
		break;

	default:
		assert(false);
	}

	return v;

 err:
	return ERR("invalid binary operation on types %s and %s",
	           value_data[l.type].body, value_data[r.type].body);
}

#define LOG(X)	  \
	do { \
		if (gc->debug) { \
			fprintf(stderr, "value operation '"X); \
			fprintf(stderr, "' on "); \
			print_debug(gc, l); \
			fprintf(stderr, "\n"); \
		} \
	} while (0)

bool
is_truthy(struct gc *gc, struct value l)
{
	LOG("truthy");

	switch (l.type) {
	case VAL_BOOL:  return l.boolean;
	case VAL_INT:   return l.integer != 0;
	case VAL_STR:   return !!strlen(gc->str[l.idx]);
	case VAL_ARRAY: return !!gc->array[l.idx]->len;
	case VAL_FLOAT: return !fcmp(l.real, 0.0);
	case VAL_REGEX: return !!gc->regex[l.idx]->num_matches;
	case VAL_NIL:   return false;
	case VAL_ERR:   return false; break;
	case VAL_FN:    return true;
	case VAL_TABLE: {
		int len = 0;
		for (int i = 0; i < TABLE_SIZE; i++)
			len += gc->table[l.idx]->bucket[i].len;
		return !!len;
	} break;

	case VAL_UNDEF:
		assert(false);
	}

	return false;
}

struct value
val_unop(struct value l, int op)
{
	switch (op) {
	case OP_ADDADD:
		if (l.type == VAL_INT) l.integer++;
		else if (l.type == VAL_FLOAT) l.real++;
		else goto err;
		return l;

	case OP_SUBSUB:
		if (l.type == VAL_INT) l.integer--;
		else if (l.type == VAL_FLOAT) l.real--;
		else goto err;
		return l;

	case OP_SUB:
		if (l.type == VAL_INT) l.integer = -l.integer;
		else if (l.type == VAL_FLOAT) l.real = -l.real;
		else goto err;
		return l;
	}

	assert(false);

 err:
	return ERR("invalid unary operation on type '%s'",
	           value_data[l.type].body);
}

struct value
value_len(struct gc *gc, struct value l)
{
	LOG("length");

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
	} else if (l.type == VAL_TABLE) {
		struct value v;
		v.type = VAL_TABLE;
		v.idx = gc_alloc(gc, VAL_TABLE);
		gc->table[v.idx] = copy_table(gc->table[l.idx]);
		l = v;
	}

	return l;
}

struct value
flip_value(struct gc *gc, struct value l)
{
	LOG("flip");
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

/* TODO: errors propagate badly around here */
static void
qsort_partition(struct gc *gc, struct value l, int begin, int end)
{
	if (begin < 0 || end < 0) return;
	if (begin >= end) return;

	int pivot = begin;
	int i = begin + 1;
	int j = begin + 1;

	for (i = begin + 1; i <= end; i++) {
		if (is_truthy(gc, val_binop(gc, gc->array[l.idx]->v[i], gc->array[l.idx]->v[pivot], OP_LESS))) {
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
	LOG("sort");
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
		if (is_truthy(gc, val_binop(gc, gc->array[v.idx]->v[i], m, OP_MORE)))
			m = gc->array[v.idx]->v[i];

	return m;
}

struct value
min_value(struct gc *gc, struct value l)
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
		if (is_truthy(gc, val_binop(gc, gc->array[v.idx]->v[i], m, OP_LESS)))
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
slice_value(struct gc *gc, struct value l, int start, int stop, int step)
{
	struct value v;
	v.type = VAL_ARRAY;
	v.idx = gc_alloc(gc, VAL_ARRAY);
	gc->array[v.idx] = new_array();

	if (stop < start && step < 0)
		for (int64_t i = start; i >= stop; i += step)
			array_push(gc->array[v.idx], gc->array[l.idx]->v[labs(i % gc->array[l.idx]->len)]);
	else if (start < stop && step < 0)
		for (int64_t i = stop; i >= start; i += step)
			array_push(gc->array[v.idx], gc->array[l.idx]->v[labs(i % gc->array[l.idx]->len)]);
	else if (start < stop && step > 0)
		for (int64_t i = start; i <= stop; i += step)
			array_push(gc->array[v.idx], gc->array[l.idx]->v[labs(i % gc->array[l.idx]->len)]);

	return v;
}

struct value
value_range(struct gc *gc, bool real, double start, double stop, double step)
{
	struct value v;
	v.type = VAL_ARRAY;
	v.idx = gc_alloc(gc, VAL_ARRAY);
	gc->array[v.idx] = new_array();

	if (fcmp(start, stop)) {
		array_push(gc->array[v.idx], INT(start));
		return v;
	}

	if (start < stop) {
		if (step <= 0) {
			return ERR("invalid range; range from %f to %f requires a positive step value (got %f)",
			           start, stop, step);
		}

		grow_array(gc->array[v.idx], (stop - start) / step + 1);
		for (double i = start; i <= stop; i += step) {
			if (real)
				array_push(gc->array[v.idx], (struct value){ VAL_FLOAT, { .real = i }, 0 });
			else
				array_push(gc->array[v.idx], (struct value){ VAL_INT, { .integer = (int64_t)i }, 0 });
		}
	} else {
		if (step >= 0) {
			return ERR("invalid range; range from %f to %f requires a negative step value (got %f)",
			           start, stop, step);
		}

		grow_array(gc->array[v.idx], (start - stop) / -step + 1);
		for (double i = start; i >= stop; i += step) {
			if (real)
				array_push(gc->array[v.idx], (struct value){ VAL_FLOAT, { .real = i }, 0 });
			else
				array_push(gc->array[v.idx], (struct value){ VAL_INT, { .integer = i }, 0 });
		}
	}

	return v;
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
