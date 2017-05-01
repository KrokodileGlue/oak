#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

#include "location.h"
#include "operator.h"

struct Token {
	struct Location loc;
	double data;
	char* value; /* the body of the token */
	struct Token* next; /* doubly-linked list */
	struct Token* prev;

	enum OpType op_type;
	enum {
		TOK_IDENTIFIER, TOK_NUMBER,
		TOK_STRING,     TOK_OPERATOR,
		TOK_SYMBOL,     TOK_INVALID
	} type;
};

void push_token(struct Location loc, int type, char* start, char* end, struct Token** prev);
void delete_tokens(struct Token* tok);
void write_tokens(FILE* fp, struct Token* tok);
void delete_token(struct Token* tok);
struct Token* rewind_token(struct Token* tok);

#endif
