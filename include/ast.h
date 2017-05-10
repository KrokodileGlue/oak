#ifndef AST_H
#define AST_H

#include <stdint.h>
#include <stdbool.h>

struct Expression;

struct Statement {
	enum StmtType {
		STMT_FN_DEF,
		STMT_FOR_LOOP,
		STMT_IF_STMT,
		STMT_WHILE_LOOP,
		STMT_EXPR
	} type;
	union {
		struct {
			struct Statement *init;
			struct Expression *cond;
			struct Expression **action; /* array of actions */
			struct Statement *body;
		} for_loop;
		struct {
			struct Expression *cond;
			struct Stmt *body;
		} if_stmt;
	};
};

struct Expression {
	enum ExprType {
		EXPR_INTEGER,
		EXPR_BOOLEAN,
		EXPR_FN_CALL,
		EXPR_LIST,
		EXPR_FLOAT
	} type;

	union {
		int64_t integer;
		bool boolean;
		double floating;
		struct Expression **list;
	};
};

#endif
