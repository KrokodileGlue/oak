#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdbool.h>

struct builtin {
	char *body;
	int prec;
	bool imp, curly;

	/* TODO: eval should be a builtin */
	enum {
		BUILTIN_SPLIT,
		BUILTIN_JOIN,
		BUILTIN_RANGE,
		BUILTIN_PUSH,
		BUILTIN_POP,
		BUILTIN_SHIFT,
		BUILTIN_INSERT,

		BUILTIN_REVERSE,
		BUILTIN_SORT,
		BUILTIN_MAP,
		BUILTIN_INT,
		BUILTIN_FLOAT,
		BUILTIN_STRING,
		BUILTIN_ABS,
		BUILTIN_COUNT,

		BUILTIN_MIN,
		BUILTIN_MAX,

		BUILTIN_CHR,
		BUILTIN_ORD,
		BUILTIN_RJUST,

		BUILTIN_HEX,
		BUILTIN_CHOMP,
		BUILTIN_TRIM,

		BUILTIN_LASTOF,

		BUILTIN_KEYS,
		BUILTIN_VALUES,

		BUILTIN_UC,
		BUILTIN_LC,
		BUILTIN_UCFIRST,
		BUILTIN_LCFIRST,

		BUILTIN_TYPE,
		BUILTIN_SAY,
		BUILTIN_SAYLN,
		BUILTIN_LENGTH
	} name;
};

extern struct builtin builtin[];
struct builtin *get_builtin(const char *name);

#endif
