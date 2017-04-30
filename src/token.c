#include "token.h"

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

void push_token(struct Location loc, int type, char* start, char* end, struct Token** prev)
{
	/* type, origin, data, value, next, prev */
	struct Token* current = malloc(sizeof (struct Token));
	if (*prev) (*prev)->next = current;

	current->type = type;
	current->loc = loc;
	current->data = 0.0;

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
		fprintf(fp, "[%s][%s]", token_type_str[tok->type], tok->value);
		if (tok->type == TOK_NUMBER) fprintf(fp, " data: %4.8f", tok->data);
		fprintf(fp, "\n");
		tok = tok->next;
	}
}
