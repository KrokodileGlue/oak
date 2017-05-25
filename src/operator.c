#include "operator.h"

static struct Operator {
	enum OpType	 type;
	char		*type_str;
	char		*value;
} operators[] = {
	{ OP_ADD,		"OP_ADD",		"+"   },
	{ OP_SUB,		"OP_SUB",		"-"   },
	{ OP_MUL,		"OP_MUL",		"*"   },
	{ OP_DIV,		"OP_DIV",		"/"   },
	{ OP_ADD_EQ,		"OP_ADD_EQ",		"+="  },
	{ OP_SUB_EQ,		"OP_SUB_EQ",		"-="  },
	{ OP_MUL_EQ,		"OP_MUL_EQ",		"*="  },
	{ OP_DIV_EQ,		"OP_DIV_EQ",		"/="  },
	{ OP_SUB_SUB,		"OP_SUB_SUB",		"--"  },
	{ OP_ADD_ADD,		"OP_ADD_ADD",		"++"  },
	{ OP_EQ,		"OP_EQ",		"="   },
	{ OP_GREATER,		"OP_GREATER",		">"   },
	{ OP_LESSER,		"OP_LESSER",		"<"   },
	{ OP_MOD,		"OP_MOD",		"%"   },
	{ OP_GREATER_EQ,	"OP_GREATER_EQ",	">="  },
	{ OP_LESSER_EQ,	"OP_LESSER_EQ",	"<="  },
	{ OP_MOD_EQ,		"OP_MOD_EQ",		"%="  },
	{ OP_EQ_EQ,		"OP_EQ_EQ",		"=="  },
	{ OP_AND,		"OP_AND",		"&"   },
	{ OP_OR,		"OP_OR",		"|"   },
	{ OP_XOR,		"OP_XOR",		"^"   },
	{ OP_NOT,		"OP_NOT",		"~"   },
	{ OP_AND_EQ,		"OP_AND_EQ",		"&="  },
	{ OP_OR_EQ,		"OP_OR_EQ",		"|="  },
	{ OP_XOR_EQ,		"OP_XOR_EQ",		"^="  },
	{ OP_NOT_EQ,		"OP_NOT_EQ",		"~="  },
	{ OP_LOGICAL_AND,	"OP_LOGICAL_AND",	"&&"  },
	{ OP_LOGICAL_OR,	"OP_LOGICAL_OR",	"||"  },
	{ OP_SHIFT_LEFT,	"OP_SHIFT_LEFT",	"<<"  },
	{ OP_SHIFT_RIGHT,	"OP_SHIFT_RIGHT",	">>"  },
	{ OP_SHIFT_LEFT_EQ,	"OP_SHIFT_LEFT_EQ",	"<<=" },
	{ OP_SHIFT_RIGHT_EQ,	"OP_SHIFT_RIGHT_EQ",	">>=" }
};

char* get_op_str(enum OpType op_type)
{
	return operators[op_type].type_str;
}

size_t get_op_len(enum OpType op_type)
{
	for (size_t i = 0; i < sizeof operators / sizeof *operators; i++) {
		if (operators[i].type == op_type) {
			return strlen(operators[op_type].value);
		}
	}
	return 0;
}

enum OpType match_operator(char *str)
{
	size_t ret = OP_INVALID, ret_len = 0;
	for (size_t i = 0; i < sizeof operators / sizeof *operators; i++) {
		if (!strncmp(str, operators[i].value, strlen(operators[i].value))) {
			ret_len = strlen(operators[i].value) >= ret_len ? strlen(operators[i].value) : ret_len;
			ret = ret_len == strlen(operators[i].value) ? i : ret;
		}
	}
	return operators[ret].type;
}
