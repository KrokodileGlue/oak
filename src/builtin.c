#include <stdlib.h>
#include <string.h>

#include "builtin.h"

struct builtin builtin[] = {
	{ "split",   1,  false, BUILTIN_SPLIT   },
	{ "join",    1,  false, BUILTIN_JOIN    },
	{ "range",   1,  false, BUILTIN_RANGE   },
	{ "push",    1,  false, BUILTIN_PUSH    },
	{ "insert",  1,  false, BUILTIN_INSERT  },

	{ "reverse", 14, false, BUILTIN_REVERSE },
	{ "sort",    14, false, BUILTIN_SORT    },
	{ "map",     14, true,  BUILTIN_MAP     },
	{ "int",     14, false, BUILTIN_INT     },
	{ "float",   14, false, BUILTIN_FLOAT   },
	{ "str",     14, false, BUILTIN_STRING  },
	{ "sum",     14, false, BUILTIN_SUM     },
	{ "abs",     14, false, BUILTIN_ABS     },
	{ "count",   14, false, BUILTIN_COUNT   },

	{ "min",     14, false, BUILTIN_MIN     },
	{ "max",     14, false, BUILTIN_MAX     },

	{ "chr",     14, false, BUILTIN_CHR     },
	{ "ord",     14, false, BUILTIN_ORD     },

	{ "hex",     14, false, BUILTIN_HEX     },
	{ "chomp",   14, false, BUILTIN_CHOMP   },

	{ "keys",    14, false, BUILTIN_KEYS    },
	{ "values",  14, false, BUILTIN_VALUES  },

	{ "uc",      14, false, BUILTIN_UC      },
	{ "lc",      14, false, BUILTIN_LC      },
	{ "ucfirst", 14, false, BUILTIN_UCFIRST },
	{ "lcfirst", 14, false, BUILTIN_LCFIRST },

	{ "type",    14, false, BUILTIN_TYPE    },
	{ "say",     14, false, BUILTIN_SAY     },
	{ "sayln",   14, false, BUILTIN_SAYLN   },
	{ "length",  14, false, BUILTIN_LENGTH  }
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
