#ifndef OPERATOR_H
#define OPERATOR_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

enum BinaryAssociation {
	ASS_LEFT,
	ASS_RIGHT,
	ASS_NONE
};

enum OpType {
	OP_LPAREN,
	OP_RPAREN,
	OP_RBRACK,
	OP_ADD,
	OP_SUB,
	OP_COMMA,
	OP_ADD_EQ,
	OP_SUB_EQ,
	OP_MUL_EQ,
	OP_DIV_EQ,
	OP_EQ,
	OP_MOD_EQ,
	OP_AND_EQ,
	OP_OR_EQ,
	OP_XOR_EQ,
	OP_NOT_EQ,
	OP_SHIFT_LEFT_EQ,
	OP_SHIFT_RIGHT_EQ,
	OP_COLON,
	OP_QUE,
	OP_NOT,
	OP_LOGICAL_OR,
	OP_OR,
	OP_XOR,
	OP_AND,
	OP_LOGICAL_AND,
	OP_EQ_EQ,
	OP_LOGICAL_NOT_EQ,
	OP_GREATER,
	OP_LESSER,
	OP_GREATER_EQ,
	OP_LESSER_EQ,
	OP_SHIFT_LEFT,
	OP_SHIFT_RIGHT,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_MUL_MUL,
	OP_SUB_SUB,
	OP_ADD_ADD,
	OP_LBRACK,
	OP_SEMICOLON,
	OP_INVALID
};

struct Operator {
	enum OpType		 type;
	char			*type_str;
	char			*value;
	enum BinaryAssociation	 ass;
	int			 prec;
};


struct Operator get_op(enum OpType type);
char *get_op_str(enum OpType op_type);
size_t get_op_len(enum OpType op_type);
enum OpType match_operator(char *str);

#endif
