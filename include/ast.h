#ifndef AST_H
#define AST_H

#include "location.h"
#include "operator.h"

#include <stdint.h>
#include <stdbool.h>

struct Expression;

enum StmtType {
	STMT_FN_DEF,
	STMT_FOR_LOOP,	// DONE
	STMT_IF_STMT,	// DONE
	STMT_WHILE_LOOP,
	STMT_EXPR,	// DONE
	STMT_VAR_DECL,	// DONE
	STMT_BLOCK,	// DONE
	STMT_PRINT,	// DONE
	STMT_INVALID
} type;

struct StatementData {
	enum StmtType	 type;
	char		*body;
};

extern struct StatementData statement_data[];

struct Statement {
	struct Token *tok;

	enum StmtType type;

	union {
		struct {
			struct Token *name;
			struct Token **args;
			size_t num;
			struct Statement *body;
		} fn_def;

		/*
		 * there are three kinds of for loops:
		 *   for expression in expression:
		 *   for vardecl; expression; expression:
		 *   for expression; expression; expression:
		 */

		struct {
			struct Statement *a;
			struct Expression *b;
			struct Expression *c;
			struct Statement *body;
		} for_loop;

		struct {
			struct Expression *cond;
			struct Statement *then;
			struct Statement *otherwise;
		} if_stmt;

		struct {
			/* a list of identifiers */
			struct Token **names;
			/* a list of initalizers */
			struct Expression **init;
			size_t num;
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
	struct Token *tok;

	enum ExprType {
		EXPR_VALUE,
		EXPR_OPERATOR,
		EXPR_FN_CALL,
		EXPR_INVALID
	} type;

	struct Operator *operator;

	union {
		struct {
			struct Expression *a, *b, *c;
			struct Expression **args;
			size_t num;
		};

		struct Token *val;
	};
};

struct Expression *mkexpr();
struct Statement  *mkstmt();
void free_ast(struct Statement **module);

#endif
