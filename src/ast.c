#include "ast.h"

struct StatementData statement_data[] = {
	{ STMT_FN_DEF,		"function definition"	},
	{ STMT_FOR_LOOP,	"for loop"		},
	{ STMT_IF_STMT,	"if"		},
	{ STMT_WHILE_LOOP,	"while loop"		},
	{ STMT_EXPR,		"expression"		},
	{ STMT_VAR_DECL,	"variable declaration"	},
	{ STMT_BLOCK,		"block"	},
	{ STMT_PRINT,		"print"	}
};
