#ifndef AST_H
#define AST_H

#include "location.h"
#include "operator.h"

#include <stdint.h>
#include <stdbool.h>

struct Expression;

struct Statement {
	struct Location loc;

	enum StmtType {
		STMT_FN_DEF,
		STMT_FOR_LOOP,
		STMT_IF_STMT,
		STMT_WHILE_LOOP,
		STMT_EXPR,
		STMT_VAR_DECL,
		STMT_BLOCK
	} type;

	union {
		struct {
			struct Statement *init;
			struct Expression *cond;
			struct Expression **action;
			struct Statement *body;
		} for_loop;

		struct {
			struct Expression *cond;
			struct Statement *body;
		} if_stmt;

		struct {
			char *name;
			struct Expression *init;
		} var_decl;

		struct {
			struct Statement **stmts;
			size_t num;
		} block;

		struct Expression *expr;
	};
};

struct Expression {
	enum ExprType {
		EXPR_VALUE,
		EXPR_OPERATOR
	} type;

	struct Operator *operator;

	union {
		struct Token *value;
		struct {
			struct Expression *a, *b, *c;
		};
	};
};

#endif
