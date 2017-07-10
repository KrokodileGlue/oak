#include <stdint.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"

uint64_t hash(char *d, size_t len)
{
	uint64_t hash = 5381;

	for (size_t i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + d[i];

	return hash;
}

struct Symbol *symbolize(struct Statement **module)
{
	struct Statement *s = module[0];
	struct Symbol *sym = oak_malloc(sizeof *sym);
	size_t i = 0;

	while (s) {
		s = module[i++];
	}

	return sym;
}
