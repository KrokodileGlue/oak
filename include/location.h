#ifndef LOCATION_H
#define LOCATION_H

#include <stdlib.h>

struct location {
	char *text;
	char *file;
	size_t index, len /* the length of the region we're looking at */;
};

#endif
