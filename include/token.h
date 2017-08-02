#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "location.h"
#include "operator.h"
#include "keyword.h"
#include "error.h"

struct token {
	struct location loc;
	bool is_line_end;
	bool is_interpolatable;

	char         *value; /* the body of the token */
	struct token *next; /* doubly-linked list */
	struct token *prev;

	enum token_type {
		TOK_IDENTIFIER, /* there's no data for identifiers, just the token body. */
		TOK_KEYWORD,
		TOK_STRING,
		TOK_SYMBOL,
		TOK_INTEGER,
		TOK_FLOAT,
		TOK_BOOL,
		TOK_END,
		TOK_INVALID
	} type;

	union {
		struct keyword *keyword;
		char           *string;
		char            symbol;
		int64_t         integer;
		double          floating;
		bool            boolean;
	};
};

void
token_push(struct location loc, enum token_type type, char *start, char *end, struct token **prev);

char *
token_get_str(enum token_type type);

void
token_clear(struct token *tok);

void
token_delete(struct token *tok);

void
token_write(struct token *tok, FILE *fp);

void
token_rewind(struct token **tok);

#endif
