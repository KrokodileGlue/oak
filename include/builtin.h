#ifndef BUILTIN_H
#define BUILTIN_H

struct builtin {
	char *body;
	int prec;

	enum {
		BUILTIN_SPLIT,
		BUILTIN_JOIN,
		BUILTIN_RANGE,
		BUILTIN_PUSH,

		BUILTIN_REVERSE,
		BUILTIN_SORT,
		BUILTIN_INT,
		BUILTIN_FLOAT,
		BUILTIN_STRING,
		BUILTIN_SUM,

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
