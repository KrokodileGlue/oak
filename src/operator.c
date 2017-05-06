#include "operator.h"

static char operators[][64] = {
	"+",  "-",  "*",  "/",
	"+=", "-=", "*=", "/=",
	"++", "--",
	"=",

	">",  "<", "%",
	">=", "<=", "%=", "==",

	"&",  "|",  "^",  "~",
	"&=", "|=", "^=", "~=",

	"&&", "||",

	">>", "<<", ">>=", "<<="
};

static char operator_type_str[][64] = {
	"OP_ADD",    "OP_SUB",    "OP_MUL",    "OP_DIV",
	"OP_ADD_EQ", "OP_SUB_EQ", "OP_MUL_EQ", "OP_DIV_EQ",
	"OP_ADD_ADD", "OP_SUB_SUB",
	"OP_EQ",

	"OP_GREATER",    "OP_LESSER",    "OP_MOD",
	"OP_GREATER_EQ", "OP_LESSER_EQ", "OP_MOD_EQ", "OP_EQ_EQ",

	"OP_AND",    "OP_OR",    "OP_XOR",    "OP_NOT",
	"OP_AND_EQ", "OP_OR_EQ", "OP_XOR_EQ", "OP_NOT_EQ",

	"OP_LOGICAL_AND", "OP_LOGICAL_OR",

	"OP_SHIFT_RIGHT",    "OP_SHIFT_LEFT",
	"OP_SHIFT_RIGHT_EQ", "OP_SHIFT_LEFT_EQ",
	"OP_INVALID"
};

char* get_op_str(enum OpType op_type)
{
	return operator_type_str[(size_t)op_type];
}

size_t get_op_len(enum OpType op_type)
{
	return strlen(operators[(size_t)op_type]);
}

enum OpType match_operator(char *str)
{
	size_t ret = OP_INVALID, ret_len = 0;
	for (size_t i = 0; i < sizeof *operators; i++) {
		if (!strncmp(str, operators[i], strlen(operators[i]))) {
			ret_len = strlen(operators[i]) >= ret_len ? strlen(operators[i]) : ret_len;
			ret = ret_len == strlen(operators[i]) ? i : ret;
		}
	}
	return (enum OpType)ret;
}
