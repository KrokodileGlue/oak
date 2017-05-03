#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>
#include <stdint.h>

#include "location.h"
#include "operator.h"

struct Token {
	struct Location loc;

	union {
		double fData;
		int64_t iData;
	};
	
	char* value; /* the body of the token */
	struct Token* next; /* doubly-linked list */
	struct Token* prev;

	enum OpType op_type;
	enum TokType {
		TOK_IDENTIFIER, TOK_KEYWORD,
		TOK_STRING,     TOK_SYMBOL,
		TOK_INTEGER,    TOK_FLOAT,
		TOK_OPERATOR,
		TOK_INVALID
	} type;
};

void token_push  (struct Location loc, enum TokType type, char* start, char* end, struct Token** prev);

void token_clear (struct Token* tok);
void token_delete(struct Token* tok);

void token_write (struct Token* tok, FILE* fp);
void token_rewind(struct Token** tok);

#endif
