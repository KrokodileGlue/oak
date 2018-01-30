#ifndef AST_H
#define AST_H

#include "location.h"
#include "operator.h"
#include "builtin.h"

#include <stdint.h>
#include <stdbool.h>

struct expression;

enum statement_type {
	STMT_FN_DEF,
	STMT_FOR_LOOP,
	STMT_IF_STMT,
	STMT_WHILE,
	STMT_DO,
	STMT_EXPR,
	STMT_VAR_DECL,
	STMT_BLOCK,
	STMT_PRINT,
	STMT_PRINTLN,
	STMT_RET,
	STMT_IMPORT,
	STMT_NULL,
	STMT_LAST,
	STMT_NEXT,
	STMT_DIE,
	STMT_LABEL,
	STMT_GOTO,
	STMT_INVALID
};

struct statement_data {
	enum statement_type  type;
	char                *body;
};

extern struct statement_data statement_data[];

struct statement {
	struct token *tok;
	enum statement_type type;
	int scope;
	struct expression *condition;

	union {
		struct {
			struct expression *expr;
		} ret;

		struct {
			char *name;
			struct token **args;
			struct expression **init;
			size_t num;
			struct statement *body;
		} fn_def;

		/*
		 * For loops are a bit weird; the particular type of
		 * for loop is encoded by whether b and c fields are
		 * non-NULL.
		 */
		struct {
			struct statement *a;
			struct expression *b;
			struct expression *c;
			struct statement *body;
		} for_loop;

		struct {
			struct expression *cond;
			struct statement *then;
			struct statement *otherwise;
		} if_stmt;

		struct {
			/* a list of identifiers */
			struct token **names;
			/* a list of initalizers */
			struct expression **init;
			size_t num;
			size_t num_init;
		} var_decl;

		struct {
			struct statement **stmts;
			size_t num;
		} block;

		struct {
			struct expression **args;
			size_t num;
		} print;

		struct {
			struct expression *cond;
			struct statement *body;
		} do_while_loop;

		struct {
			struct expression *cond;
			struct statement *body;
		} while_loop;

		struct {
			struct token *name;
			struct token *as;
		} import;

		char *label;
		struct expression *expr;
	};
};

struct expression {
	struct token *tok;

	enum expression_type {
		EXPR_VALUE,
		EXPR_OPERATOR,
		EXPR_FN_CALL,
		EXPR_FN_DEF,
		EXPR_LIST,
		EXPR_LIST_COMPREHENSION,
		EXPR_SUBSCRIPT,
		EXPR_MAP,
		EXPR_BUILTIN,
		EXPR_REGEX,
		EXPR_GROUP,
		EXPR_EVAL,
		EXPR_TABLE,
		EXPR_VARARGS,
		EXPR_MATCH,
		EXPR_INVALID
	} type;

	struct operator *operator;
	struct builtin *bi;

	union {
		struct {
			struct expression *a, *b, *c;

			/*
			 * Used for function arguments, list, and lots
			 * of other stuff.
			 */
			struct expression **args;
			struct expression **match;
			struct statement **bodies;

			struct token **keys;
			struct statement *s;
			size_t num;
		};

		struct token *val;
	};
};

struct expression *new_expression(struct token *tok);
struct statement *new_statement(struct token *tok);
void free_ast(struct statement **module);
void print_ast(FILE *f, struct statement **module);
void free_stmt(struct statement *s);

#endif
