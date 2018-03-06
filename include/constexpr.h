#ifndef CONSTEXPR_H
#define CONSTEXPR_H

#include <stdbool.h>
#include "value.h"
#include "compile.h"

bool is_constant_expr(struct compiler *c, struct symbol *sym, struct expression *e);
struct value compile_constant_expr(struct compiler *c, struct symbol *sym, struct expression *e);

#endif
