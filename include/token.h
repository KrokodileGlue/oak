#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>
#include <stdint.h>

#include "location.h"
#include "operator.h"
#include "keyword.h"
#include "error.h"

struct Token {
	struct Location loc;

	char *value; /* the body of the token */
	struct Token *next; /* doubly-linked list */
	struct Token *prev;

	enum TokType {
		TOK_IDENTIFIER, /* there's no data for identifiers, just the token body. */
		TOK_KEYWORD,
		TOK_STRING,
		TOK_SYMBOL,
		TOK_INTEGER,
		TOK_FLOAT,
		TOK_OPERATOR,
		TOK_BOOL,
		TOK_END,
		TOK_INVALID
	} type;

	union {
		enum KeywordType	 keyword;
		char			*string;
		char			 symbol;
		int64_t		 integer;
		double			 floating;
		struct Operator	*operator;
		bool			 boolean;
	};
};

void token_push  (struct Location loc, enum TokType type, char *start, char *end, struct Token **prev);
char *token_get_str(enum TokType type);

void token_clear (struct Token *tok);
void token_delete(struct Token *tok);

void token_write (struct Token *tok, FILE *fp);
void token_rewind(struct Token **tok);

#endif
