#ifndef AST_H
#define AST_H

#include "location.h"
#include "operator.h"

#include <stdint.h>
#include <stdbool.h>

struct Expression;

enum StmtType {
	STMT_FN_DEF,
	STMT_FOR_LOOP,
	STMT_IF_STMT,
	STMT_WHILE_LOOP,
	STMT_EXPR,
	STMT_VAR_DECL,
	STMT_BLOCK,
	STMT_PRINT
} type;

struct StatementData {
	enum StmtType	 type;
	char		*body;
};

extern struct StatementData statement_data[];

struct Statement {
	struct Location loc;

	enum StmtType type;

	union {
		struct {
			struct Statement *init;
			struct Expression *cond;
			struct Expression **action;
			struct Statement *body;
		} for_loop;

		struct {
			struct Expression *cond;
			struct Statement *then;
			struct Statement *otherwise;
		} if_stmt;

		struct {
			char *name;
			struct Expression *init;
		} var_decl;

		struct {
			struct Statement **stmts;
			size_t num;
		} block;

		struct {
			struct Expression **args;
			size_t num;
		} print;

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
