#include <stdlib.h>
#include <string.h>

#include "builtin.h"

struct builtin builtin[] = {
	{ "split",   1,  BUILTIN_SPLIT   },
	{ "join",    1,  BUILTIN_JOIN    },
	{ "range",   1,  BUILTIN_RANGE   },
	{ "push",    1,  BUILTIN_PUSH    },

	{ "reverse", 14, BUILTIN_REVERSE },
	{ "sort",    14, BUILTIN_SORT    },
	{ "int",     14, BUILTIN_INT     },
	{ "float",   14, BUILTIN_FLOAT   },
	{ "str",     14, BUILTIN_STRING  },
	{ "sum",     14, BUILTIN_SUM     },
	{ "abs",     14, BUILTIN_ABS     },
	{ "count",   14, BUILTIN_COUNT   },

	{ "uc",      14, BUILTIN_UC      },
	{ "lc",      14, BUILTIN_LC      },
	{ "ucfirst", 14, BUILTIN_UCFIRST },
	{ "lcfirst", 14, BUILTIN_LCFIRST },

	{ "type",    14, BUILTIN_TYPE    },
	{ "say",     14, BUILTIN_SAY     },
	{ "sayln",   14, BUILTIN_SAYLN   },
	{ "length",  14, BUILTIN_LENGTH  }
};

struct builtin *
get_builtin(const char *body)
{
	for (size_t i = 0; i < sizeof builtin / sizeof builtin[0]; i++) {
		if (!strcmp(builtin[i].body, body))
			return &builtin[i];
	}

	return NULL;
}
