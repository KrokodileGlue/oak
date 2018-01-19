#ifndef BUILTIN_H
#define BUILTIN_H

struct builtin {
	char *body;

	enum {
		BUILTIN_SPLIT,
		BUILTIN_JOIN,
		BUILTIN_REVERSE,
		BUILTIN_UC,
		BUILTIN_LC,
		BUILTIN_UCFIRST,
		BUILTIN_LCFIRST,
	} name;
};

extern struct builtin builtin[];
struct builtin *get_builtin(const char *name);

#endif
