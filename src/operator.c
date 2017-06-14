#include "operator.h"

static struct Operator operators[] = {
	{ OP_LPAREN,		"OP_LPAREN",		"(",	ASS_NONE,	0 },
	{ OP_RPAREN,		"OP_RPAREN",		")",	ASS_NONE,	1 },
	{ OP_ADD,		"OP_ADD",		"+",	ASS_LEFT,	1 },
	{ OP_SUB,		"OP_SUB",		"-",	ASS_LEFT,	1 },
	{ OP_COMMA,		"OP_COMMA",		",",	ASS_NONE,	2 },
	{ OP_ADD_EQ,		"OP_ADD_EQ",		"+=",	ASS_LEFT,	4 },
	{ OP_SUB_EQ,		"OP_SUB_EQ",		"-=",	ASS_LEFT,	4 },
	{ OP_MUL_EQ,		"OP_MUL_EQ",		"*=",	ASS_LEFT,	4 },
	{ OP_DIV_EQ,		"OP_DIV_EQ",		"/=",	ASS_LEFT,	4 },
	{ OP_EQ,		"OP_EQ",		"=",	ASS_LEFT,	4 },
	{ OP_MOD_EQ,		"OP_MOD_EQ",		"%=",	ASS_NONE,	4 },
	{ OP_AND_EQ,		"OP_AND_EQ",		"&=",	ASS_NONE,	4 },
	{ OP_OR_EQ,		"OP_OR_EQ",		"|=",	ASS_NONE,	4 },
	{ OP_XOR_EQ,		"OP_XOR_EQ",		"^=",	ASS_NONE,	4 },
	{ OP_NOT_EQ,		"OP_NOT_EQ",		"~=",	ASS_NONE,	4 },
	{ OP_SHIFT_LEFT_EQ,	"OP_SHIFT_LEFT_EQ",	"<<=",	ASS_NONE,	4 },
	{ OP_SHIFT_RIGHT_EQ,	"OP_SHIFT_RIGHT_EQ",	">>=",	ASS_NONE,	4 },
	{ OP_COLON,		"OP_COLON",		":",	ASS_RIGHT,	5 },
	{ OP_QUE,		"OP_QUESTION",		"?",	ASS_LEFT,	6 },
	{ OP_NOT,		"OP_NOT",		"~",	ASS_NONE,	10 },
	{ OP_LOGICAL_OR,	"OP_LOGICAL_OR",	"||",	ASS_NONE,	12 },
	{ OP_OR,		"OP_OR",		"|",	ASS_NONE,	12 },
	{ OP_XOR,		"OP_XOR",		"^",	ASS_NONE,	14 },
	{ OP_AND,		"OP_AND",		"&",	ASS_NONE,	16 },
	{ OP_LOGICAL_AND,	"OP_LOGICAL_AND",	"&&",	ASS_NONE,	16 },
	{ OP_EQ_EQ,		"OP_EQ_EQ",		"==",	ASS_NONE,	18 },
	{ OP_LOGICAL_NOT_EQ,	"OP_LOGICAL_NOT_EQ",	"!=",	ASS_NONE,	18 },
	{ OP_GREATER,		"OP_GREATER",		">",	ASS_NONE,	20 },
	{ OP_LESSER,		"OP_LESSER",		"<",	ASS_NONE,	20 },
	{ OP_GREATER_EQ,	"OP_GREATER_EQ",	">=",	ASS_NONE,	20 },
	{ OP_LESSER_EQ,	"OP_LESSER_EQ",	"<=",	ASS_NONE,	20 },
	{ OP_SHIFT_LEFT,	"OP_SHIFT_LEFT",	"<<",	ASS_NONE,	22 },
	{ OP_SHIFT_RIGHT,	"OP_SHIFT_RIGHT",	">>",	ASS_NONE,	22 },
	{ OP_MUL,		"OP_MUL",		"*",	ASS_LEFT,	26 },
	{ OP_DIV,		"OP_DIV",		"/",	ASS_LEFT,	26 },
	{ OP_MOD,		"OP_MOD",		"%",	ASS_LEFT,	26 },
	{ OP_MUL_MUL,		"OP_MUL_MUL",		"**",	ASS_LEFT,	28 },
	{ OP_SUB_SUB,		"OP_SUB_SUB",		"--",	ASS_LEFT,	32 },
	{ OP_ADD_ADD,		"OP_ADD_ADD",		"++",	ASS_LEFT,	32 },
	{ OP_LBRACK,		"OP_LBRACK",		"[",	ASS_RIGHT,	32 },
	{ OP_INVALID,		"OP_INVALID",		"",	ASS_NONE,	0 }
};

struct Operator get_op(enum OpType type)
{
	return operators[type];
}

char *get_op_str(enum OpType op_type)
{
	return operators[op_type].type_str;
}

size_t get_op_len(enum OpType op_type)
{
	return strlen(operators[op_type].value);
}

enum OpType match_operator(char *str)
{
	size_t ret = OP_INVALID, ret_len = 0;
	for (size_t i = 0; i < sizeof operators / sizeof *operators - 1; i++) {
		if (!strncmp(str, operators[i].value,
			    strlen(operators[i].value))) {
			ret_len = strlen(operators[i].value) >= ret_len
				? strlen(operators[i].value) : ret_len;
			ret = ret_len == strlen(operators[i].value) ? i : ret;
		}
	}
	return operators[ret].type;
}
