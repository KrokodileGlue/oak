#include <stdint.h>
#include <stdlib.h>

uint64_t hash(char *d, size_t len)
{
	uint64_t hash = 5381;

	for (size_t i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + d[i];

	return hash;
}
