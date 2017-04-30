#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

#include "location.h"

static char operators[][64] = {
	"+",  "-",  "*",  "/",
	"+=", "-=", "*=", "/=",

	">",  "<", "%",
	">=", "<=", "%=", "==",

	"&",  "|",  "^",  "~",
	"&=", "|=", "^=", "~=",

	"&&", "||",

	">>", "<<", ">>=", "<<="
};

struct Token {
	struct Location loc;
	double data;
	char* value; /* the body of the token */
	struct Token* next; /* doubly-linked list */
	struct Token* prev;

	enum {
		TOK_IDENTIFIER, TOK_NUMBER,
		TOK_STRING,     TOK_OPERATOR,
		TOK_SYMBOL,     TOK_INVALID
	} type;
	
	enum {
		OP_ADD,    OP_SUB,    OP_MUL,    OP_DIV,
		OP_ADD_EQ, OP_SUB_EQ, OP_MUL_EQ, OP_DIV_EQ,

		OP_GREATER,    OP_LESSER,    OP_MOD,
		OP_GREATER_EQ, OP_LESSER_EQ, OP_MOD_EQ, OP_EQ_EQ,

		OP_AND,    OP_OR,    OP_XOR,    OP_NOT,
		OP_AND_EQ, OP_OR_EQ, OP_XOR_EQ, OP_NOT_EQ,

		OP_LOGICAL_AND, OP_LOGICAL_OR,

		OP_SHIFT_LEFT,    OP_SHIFT_RIGHT,
		OP_SHIFT_LEFT_EQ, OP_SHIFT_RIGHT_EQ
	} op_type;
};

void push_token(struct Location loc, int type, char* start, char* end, struct Token** prev);
void delete_tokens(struct Token* tok);
void write_tokens(FILE* fp, struct Token* tok);
void delete_token(struct Token* tok);
struct Token* rewind_token(struct Token* tok);

#endif
