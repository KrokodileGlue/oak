#include <stdlib.h>
#include <string.h>

#include "builtin.h"

struct builtin builtin[] = {
	{ "split",   1,  BUILTIN_SPLIT   },
	{ "join",    1,  BUILTIN_JOIN    },
	{ "reverse", 14, BUILTIN_REVERSE },
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
