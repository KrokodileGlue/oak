#ifndef MACHINE_H
#define MACHINE_H

#include <stdlib.h>
#include "constant.h"
#include "gc.h"

/* This is kinda retarded. */
#define MAX_CALL_DEPTH 1024

struct vm {
	struct instruction *code;
	size_t ip;

	/* Return locations. */
	int *callstack;

	/* Detailed information about each call. */
	struct value calls[MAX_CALL_DEPTH];
	int args[MAX_CALL_DEPTH];
	size_t csp;

	struct value *imp;
	size_t impp;

	struct value *stack;
	size_t sp;
	size_t maxsp;

	struct value **frame;
	size_t fp;
	size_t maxfp;

	struct constant_table *ct;
	struct reporter *r;
	struct gc *gc;

	struct ktre *re;
	char *subject;

	bool debug;
	FILE *f;
};

#endif
