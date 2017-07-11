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
	STMT_WHILE,
	STMT_DO,
	STMT_EXPR,
	STMT_VAR_DECL,
	STMT_BLOCK,
	STMT_PRINT,
	STMT_YIELD,
	STMT_CLASS,
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
			struct Expression *expr;
		} yield;

		struct {
			char *name;
			struct Token **args;
			size_t num;
			struct Statement *body;
		} fn_def;

		/*
		 * for loops are a bit weird, for-each and c-style loops are distinguished
		 * by the c field; if it's NULL then it's for-each, otherwise it's c-style.
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
			size_t num_init;
		} var_decl;

		struct {
			struct Statement **stmts;
			size_t num;
		} block;

		struct {
			struct Expression **args;
			size_t num;
		} print;

		struct {
			struct Expression *cond;
			struct Statement *body;
		} do_while_loop;

		struct {
			struct Expression *cond;
			struct Statement *body;
		} while_loop;

		struct {
			char *name;
			/* the name of the class this class inherits from */
			struct Token *parent_name;
			struct Statement **body;
			size_t num;
		} class;

		struct Expression *expr;
	};
};

struct Expression {
	struct Token *tok;

	enum ExprType {
		EXPR_VALUE,
		EXPR_OPERATOR,
		EXPR_FN_CALL,
		EXPR_LIST,
		EXPR_LIST_COMPREHENSION,
		EXPR_SUBSCRIPT,
		EXPR_INVALID
	} type;

	struct Operator *operator;

	union {
		struct {
			struct Expression *a, *b, *c;
			struct Expression **args; /* used for function arguments and lists */
			size_t num;
		};

		struct Token *val;
	};
};

struct Expression *mkexpr(struct Token *tok);
struct Statement  *mkstmt(struct Token *tok);
void free_ast(struct Statement **module);
void print_ast(FILE *f, struct Statement **module);

#endif
