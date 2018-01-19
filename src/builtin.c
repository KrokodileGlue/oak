#include <stdlib.h>
#include <string.h>

#include "builtin.h"

struct builtin builtin[] = {
	{ "split",   BUILTIN_SPLIT },
	{ "join",    BUILTIN_JOIN },
	{ "reverse", BUILTIN_REVERSE },
	{ "uc",      BUILTIN_UC },
	{ "lc",      BUILTIN_LC },
	{ "ucfirst", BUILTIN_UCFIRST },
	{ "lcfirst", BUILTIN_LCFIRST }
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
