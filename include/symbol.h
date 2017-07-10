#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdint.h>
#include "ast.h"

struct Symbol {
	uint64_t id;
	char *name;

	enum SymbolType {
		SYM_FN,
		SYM_VAR
	} type;
};

struct Symbol *symbolize(struct Statement **module);

#endif
