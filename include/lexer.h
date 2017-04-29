#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

struct Token {
	enum {
		TOK_IDENTIFIER, TOK_INTEGER,
		TOK_STRING,     TOK_OPERATOR,
		TOK_SYMBOL,     TOK_INVALID
	} type;

	size_t origin; /* the index of the token's original location in the source file */
	union {
		double fData;
		enum {
			OP_ADD,    OP_SUB,    OP_MUL,    OP_DIV,
			OP_ADD_EQ, OP_SUB_EQ, OP_MUL_EQ, OP_DIV_EQ,

			OP_GREATER,    OP_LESSER,
			OP_GREATER_EQ, OP_LESSER_EQ,

			OP_AND,    OP_OR,    OP_XOR,    OP_NOT,
			OP_AND_EQ, OP_OR_EQ, OP_XOR_EQ, OP_NOT_EQ,

			OP_LOGICAL_AND, OP_LOGICAL_OR,

			OP_SHIFT_LEFT,    OP_SHIFT_RIGHT,
			OP_SHIFT_LEFT_EQ, OP_SHIFT_RIGHT_EQ
		} OpData;
	};

	char* value; /* the body of the token */
	
	struct Token* next; /* doubly-linked list */
	struct Token* prev;
};

struct Token* tokenize(char* code);
void write_tokens(FILE* fp, struct Token* tok);

#endif
