#include <string.h>

#include "util.h"
#include "machine.h"
#include "value.h"

struct frame *new_frame(size_t num_variables)
{
	struct frame *f = oak_malloc(sizeof *f);

	memset(f, 0, sizeof *f);
	f->vars = oak_malloc(sizeof f->vars[0] * num_variables);
	memset(f->vars, 0, sizeof f->vars[0] * num_variables);

	return f;
}
