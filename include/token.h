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

	union {
		double fData;
		int64_t iData;
		enum OpType op_type;
		enum KeywordType keyword_type;
		bool bool_type;
	};
	
	char *value; /* the body of the token */
	struct Token *next; /* doubly-linked list */
	struct Token *prev;

	enum TokType {
		TOK_IDENTIFIER, TOK_KEYWORD,
		TOK_STRING,     TOK_SYMBOL,
		TOK_INTEGER,    TOK_FLOAT,
		TOK_OPERATOR,   TOK_BOOL,
		TOK_INVALID
	} type;
};

void token_push  (struct Location loc, enum TokType type, char *start, char *end, struct Token **prev);

void token_clear (struct Token *tok);
void token_delete(struct Token *tok);

void token_write (struct Token *tok, FILE *fp);
void token_rewind(struct Token **tok);

void token_match(struct ErrorState *es, struct Token **token, enum TokType tok_type, const char *str);
void token_expect(struct ErrorState *es, struct Token **token, enum TokType tok_type, const char *str);

#endif
