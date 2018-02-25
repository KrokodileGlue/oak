#ifndef MACHINE_H
#define MACHINE_H

#include <stdlib.h>
#include "constant.h"
#include "gc.h"
#include "oak.h"

/* This is kinda retarded. */
#define MAX_CALL_DEPTH 8192

/* TODO: refucktor frames */

struct vm {
	struct instruction *code;
	size_t ip;
	size_t step;

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
	int *module;
	size_t fp;
	size_t maxfp;

	struct constant_table *ct;
	struct reporter *r;
	struct gc *gc;

	struct ktre *re;
	int match;
	char *subject;

	bool debug;
	FILE *f;
	bool returning;
	bool escaping;

	struct oak *k;
	struct module *m;
};

#endif
