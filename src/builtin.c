#include <stdlib.h>
#include <string.h>

#include "builtin.h"

struct builtin builtin[] = {
	{ "split",   1,  false, false, BUILTIN_SPLIT   },
	{ "join",    1,  false, false, BUILTIN_JOIN    },
	{ "range",   1,  false, false, BUILTIN_RANGE   },
	{ "push",    1,  false, false, BUILTIN_PUSH    },
	{ "pop",     1,  false, false, BUILTIN_POP     },
	{ "shift",   1,  false, false, BUILTIN_SHIFT   },
	{ "insert",  1,  false, false, BUILTIN_INSERT  },

	{ "reverse", 14, false, false, BUILTIN_REVERSE },
	{ "sort",    14, false, true,  BUILTIN_SORT    },
	{ "map",     14, true,  true,  BUILTIN_MAP     },
	{ "int",     14, false, false, BUILTIN_INT     },
	{ "float",   14, false, false, BUILTIN_FLOAT   },
	{ "str",     14, false, false, BUILTIN_STRING  },
	{ "sum",     14, false, false, BUILTIN_SUM     },
	{ "abs",     14, false, false, BUILTIN_ABS     },
	{ "count",   14, false, false, BUILTIN_COUNT   },

	{ "min",     14, false, false, BUILTIN_MIN     },
	{ "max",     14, false, false, BUILTIN_MAX     },

	{ "chr",     14, false, false, BUILTIN_CHR     },
	{ "ord",     14, false, false, BUILTIN_ORD     },
	{ "rjust",   14, false, false, BUILTIN_RJUST   },

	{ "hex",     14, false, false, BUILTIN_HEX     },
	{ "chomp",   14, false, false, BUILTIN_CHOMP   },
	{ "trim",    14, false, false, BUILTIN_TRIM    },

	{ "lastof",  14, false, false, BUILTIN_LASTOF  },

	{ "keys",    14, false, false, BUILTIN_KEYS    },
	{ "values",  14, false, false, BUILTIN_VALUES  },

	{ "uc",      14, false, false, BUILTIN_UC      },
	{ "lc",      14, false, false, BUILTIN_LC      },
	{ "ucfirst", 14, false, false, BUILTIN_UCFIRST },
	{ "lcfirst", 14, false, false, BUILTIN_LCFIRST },

	{ "type",    14, false, true,  BUILTIN_TYPE    },
	{ "say",     14, false, true,  BUILTIN_SAY     },
	{ "sayln",   14, false, true,  BUILTIN_SAYLN   },
	{ "length",  14, false, true,  BUILTIN_LENGTH  }
};

struct builtin *
get_builtin(const char *body)
{
	for (size_t i = 0; i < sizeof builtin / sizeof *builtin; i++)
		if (!strcmp(builtin[i].body, body))
			return &builtin[i];

	return NULL;
}
