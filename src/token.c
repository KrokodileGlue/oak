#include "token.h"
#include "operator.h"

#include <stdlib.h>
#include <string.h>

static char token_type_str[][64] = {
	"IDENTIFIER",
	"NUMBER    ",
	"STRING    ",
	"OPERATOR  ",
	"SYMBOL    ",
	"INVALID   "
};

struct Token* rewind_token(struct Token* tok)
{
	if (tok)
		while (tok->prev) tok = tok->prev;
	return tok;
}

void delete_token(struct Token* tok)
{
	struct Token* prev = tok->prev;
	struct Token* next = tok->next;
	if (prev) prev->next = next;
	if (next) next->prev = prev;
	free(tok->value);
	free(tok);
}

void push_token(struct Location loc, int type, char* start, char* end, struct Token** prev)
{
	/* type, origin, data, value, next, prev */
	struct Token* current = malloc(sizeof (struct Token));
	if (*prev) (*prev)->next = current;

	current->type = type;
	current->loc = loc;
	current->data = 0.0;
	current->op_type = OP_INVALID;

	current->value = malloc(end - start + 1);
	strncpy(current->value, start, end - start);
	current->value[end - start] = 0;

	current->next = NULL;
	current->prev = *prev;

	*prev = current;
	current = current->next;
}

void delete_tokens(struct Token* tok)
{
	/* rewind the token stream */
	if (tok)
		while (tok->prev) tok = tok->prev;

	while (tok) {
		if (tok->next) {
			free(tok->value);
			tok = tok->next;
			if (tok && tok->prev) free(tok->prev);
		} else {
			free(tok->value);
			free(tok);
			tok = NULL;
		}
	}
}

void write_tokens(FILE* fp, struct Token* tok)
{
	while (tok) {
		fprintf(fp, "[%s]", token_type_str[tok->type]);
		if (tok->type == TOK_OPERATOR) fprintf(fp, "[%16s]", get_op_str(tok->op_type));
		if (tok->type == TOK_NUMBER) fprintf(fp, "[%16.8f]", tok->data);
		fprintf(fp, "[%s]", tok->value);
		fprintf(fp, "\n");
		tok = tok->next;
	}
}
