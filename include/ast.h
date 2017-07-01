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
	STMT_PRINT,
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
		/* there are three kinds of for loops:
		 *   for expression in expression:
		 *   for vardecl; expression; expression:
		 *   for expression; expression; expression:
		 */

		struct {
			enum {
				STMT_FOR_EACH,
				STMT_FOR_CSTYLE
			} type;
			union {
				struct Statement *vardecl;
				struct Expression *a;
			};
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
		EXPR_INVALID
	} type;

	struct Operator *operator;

	union {
		struct {
			struct Expression *a, *b, *c;
		};

		struct Token *value;
	};
};

struct Expression *mkexpr();
struct Statement  *mkstmt();
void free_ast(struct Statement **module);

#endif
