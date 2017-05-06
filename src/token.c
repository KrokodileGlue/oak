#include "token.h"
#include "operator.h"

#include <stdlib.h>
#include <string.h>

static char token_type_str[][64] = {
	"IDENTIFIER", "KEYWORD",
	"STRING",     "SYMBOL",
	"INTEGER",    "FLOAT",
	"OPERATOR",
	"INVALID"
};

void token_rewind(struct Token **tok)
{
	if (*tok)
		while ((*tok)->prev)
			(*tok) = (*tok)->prev;
}

void token_delete(struct Token *tok)
{
	struct Token *prev = tok->prev;
	struct Token *next = tok->next;

	if (prev) prev->next = next;
	if (next) next->prev = prev;

	free(tok->value);
	free(tok);
}

void token_clear(struct Token *tok)
{
	token_rewind(&tok);

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

void token_push(struct Location loc, enum TokType type, char *start, char *end, struct Token **prev)
{
	struct Token *current = malloc(sizeof (struct Token));
	if (*prev) (*prev)->next = current;

	current->type = type;
	current->loc = loc;
	current->iData = 0;
	current->op_type = OP_INVALID;

	current->value = malloc(end - start + 1);
	strncpy(current->value, start, end - start);
	current->value[end - start] = 0;

	current->next = NULL;
	current->prev = *prev;

	*prev = current;
	current = current->next;
}

void token_write(struct Token *tok, FILE *fp)
{
	while (tok) {
		fprintf(fp, "[%10s]", token_type_str[(size_t)tok->type]);

		if (tok->type == TOK_OPERATOR) fprintf(fp, "[%17s]", get_op_str(tok->op_type));
		if (tok->type == TOK_FLOAT) fprintf(fp, "[%17.4f]", tok->fData);
		if (tok->type == TOK_INTEGER) fprintf(fp, "[%17zd]", tok->iData);

		fprintf(fp, "[%s]", tok->value);
		fprintf(fp, "\n");
		tok = tok->next;
	}
}
