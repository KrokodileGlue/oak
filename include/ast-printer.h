#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include <stdio.h>
#include <ast.h>

void print_ast(FILE *f, struct Statement **module);

#endif
