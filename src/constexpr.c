#include <assert.h>

#include "constexpr.h"
#include "token.h"
#include "util.h"

bool
is_constant_expr(struct compiler *c, struct symbol *sym, struct expression *e)
{
	if (!e) return false;
	bool ret = true;

	switch (e->type) {
	case EXPR_OPERATOR:
		switch (e->operator->type) {
		case OPTYPE_INVALID: assert(false);
		case OPTYPE_FN_CALL:
		case OPTYPE_BINARY:
			if (e->operator->name == OP_CC
			    || e->operator->name == OP_SQUIGGLEEQ) ret = false;
			else ret = is_constant_expr(c, sym, e->a)
				     && is_constant_expr(c, sym, e->b);
			break;

		case OPTYPE_SUBSCRIPT:
			ret = is_constant_expr(c, sym, e->a)
				&& is_constant_expr(c, sym, e->b)
				&& e->c ? is_constant_expr(c, sym, e->c) : true
				&& e->d ? is_constant_expr(c, sym, e->d) : true;
			break;

		case OPTYPE_POSTFIX:
		case OPTYPE_PREFIX:
			ret = is_constant_expr(c, sym, e->a);
			break;

		case OPTYPE_TERNARY:
			ret = is_constant_expr(c, sym, e->a)
				&& is_constant_expr(c, sym, e->b)
				&& is_constant_expr(c, sym, e->c);
			break;
		}
		break;

	case EXPR_SUBSCRIPT:
		ret = is_constant_expr(c, sym, e->a)
			&& is_constant_expr(c, sym, e->b);
		break;

	/*
	 * Some of these are only forbidden because they'd _really_
	 * annoying to implement.
	 */
	case EXPR_MUTATOR:
	case EXPR_BUILTIN:
	case EXPR_MATCH:
	case EXPR_EVAL:
	case EXPR_LIST_COMPREHENSION:
	case EXPR_REGEX:
	case EXPR_VARARGS:
	case EXPR_FN_DEF:
	case EXPR_GROUP: ret = false; break;

	case EXPR_FN_CALL:
		ret = is_constant_expr(c, sym, e->a)
			&& is_constant_expr(c, sym, e->b);
		/* Fall through */
	case EXPR_LIST:
	case EXPR_TABLE:
		for (size_t i = 0; i < e->num && ret; i++)
			ret = ret && is_constant_expr(c, sym, e->args[i]);
		break;

	case EXPR_INVALID: assert(false);
	case EXPR_SLICE:
		ret = is_constant_expr(c, sym, e->a)
			&& (e->b ? is_constant_expr(c, sym, e->b) : true)
			&& (e->c ? is_constant_expr(c, sym, e->c) : true)
			&& (e->d ? is_constant_expr(c, sym, e->d) : true);
		break;

	case EXPR_VALUE:
		switch (e->val->type) {
		case TOK_IDENTIFIER: {
			struct symbol *var = resolve(sym, e->val->value);
			if (!var) ret = false;
			else ret = (var->type == SYM_ENUM);
		} break;

		case TOK_STRING:
			ret = !e->val->is_interpolatable;
			break;

		default: ret = true;
		}
		break;
	}

	/* DOUT("is_constant_expr? %s --- %s", e->tok->value, ret ? "true" : "false"); */

	return ret;
}

struct value
compile_constant_operator(struct compiler *c, struct symbol *sym, struct expression *e)
{
	struct value v = NIL;

	switch (e->operator->type) {
	case OPTYPE_INVALID: assert(false);
	case OPTYPE_BINARY:
		switch (e->operator->name) {
		case OP_ARROW: {
			struct value a = compile_constant_expr(c, sym, e->a);
			struct value b = compile_constant_expr(c, sym, e->b);

			double start = a.type == VAL_INT ? (double)a.integer : a.real;
			double stop = b.type == VAL_INT ? (double)b.integer : b.real;

			v = value_range(c->gc, a.type == VAL_FLOAT || b.type == VAL_FLOAT, start, stop, 1.0);
		} break;

		default:
			v = val_binop(c->gc, compile_constant_expr(c, sym, e->a), compile_constant_expr(c, sym, e->b), e->operator->name);
		}
		break;

	case OPTYPE_PREFIX:
		switch (e->operator->name) {
		case OP_ADD:
			v = compile_constant_expr(c, sym, e->a);
			break;

		default:
			v = val_unop(compile_constant_expr(c, sym, e->a), e->operator->name);
		}
		break;

	case OPTYPE_TERNARY:
		if (is_truthy(c->gc, compile_constant_expr(c, sym, e->a)))
			v = compile_constant_expr(c, sym, e->b);
		else v = compile_constant_expr(c, sym, e->c);
		break;

	default:
		DOUT("unimplemented constant operator compiler for operator of type %d", e->operator->type);
		assert(false);
	}

	return v;
}

struct value
compile_constant_expr(struct compiler *c, struct symbol *sym, struct expression *e)
{
	assert(e);
	struct value v = NIL;

	switch (e->type) {
	case EXPR_OPERATOR:
		v = compile_constant_operator(c, sym, e);
		break;

	case EXPR_LIST:
		v.type = VAL_ARRAY;
		v.idx = gc_alloc(c->gc, VAL_ARRAY);
		c->gc->array[v.idx] = new_array();

		for (size_t i = 0; i < e->num; i++)
			array_push(c->gc->array[v.idx], compile_constant_expr(c, sym, e->args[i]));
		break;

	case EXPR_TABLE:
		v.type = VAL_TABLE;
		v.idx = gc_alloc(c->gc, VAL_TABLE);
		c->gc->table[v.idx] = new_table();

		for (size_t i = 0; i < e->num; i++)
			table_add(c->gc->table[v.idx], e->keys[i]->value, compile_constant_expr(c, sym, e->args[i]));
		break;

	case EXPR_VALUE:
		switch (e->val->type) {
		case TOK_IDENTIFIER: {
			struct symbol *var = resolve(sym, e->val->value);

			if (var->type == SYM_ENUM) v = INT(var->_enum);
			else assert(false);
		} break;

		default:
			v = make_value_from_token(c, e->val);
		}
		break;

	case EXPR_SLICE: {
		struct value a = compile_constant_expr(c, sym, e->a);
		struct value C = e->b ? compile_constant_expr(c, sym, e->b) : INT(0);
		struct value d = e->c ? compile_constant_expr(c, sym, e->c) : INT(c->gc->array[a.idx]->len);
		struct value E = e->d ? compile_constant_expr(c, sym, e->d) : INT(1);

		int64_t start = C.type == VAL_INT ? C.integer : 0;
		int64_t stop = d.type == VAL_INT ? d.integer : c->gc->array[a.idx]->len - 1;
		int64_t step = E.type == VAL_INT ? E.integer : 1;
		v = slice_value(c->gc, a, start, stop, step);
	} break;

	default:
		DOUT("unimplemented constant compiler for expression of type %d", e->type);
		assert(false);
	}

	return v;
}
