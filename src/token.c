#include "token.h"
#include "operator.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

static char token_type_str[][64] = {
	"IDENTIFIER",
	"KEYWORD",
	"STRING",
	"SYMBOL",
	"INTEGER",
	"FLOAT",
	"BOOL",
	"END",
	"INVALID"
};

void token_rewind(struct token **tok)
{
	if (*tok)
		while ((*tok)->prev)
			(*tok) = (*tok)->prev;
}

void token_delete(struct token *tok)
{
	struct token *prev = tok->prev;
	struct token *next = tok->next;

	if (prev) prev->next = next;
	if (next) next->prev = prev;

	free(tok->value);
	if (tok->type == TOK_STRING) free(tok->string);
	free(tok);
}

void token_clear(struct token *tok)
{
	token_rewind(&tok);

	while (tok) {
		if (tok->next) {
			free(tok->value);
			if (tok->type == TOK_STRING) free(tok->string);
			tok = tok->next;
			if (tok && tok->prev) free(tok->prev);
		} else {
			free(tok->value);
			if (tok->type == TOK_STRING) free(tok->string);
			free(tok);
			tok = NULL;
		}
	}
}

void token_push(struct location loc, enum token_type type, char *start, char *end, struct token **prev)
{
	struct token *current = oak_malloc(sizeof (struct token));
	if (*prev) (*prev)->next = current;

	current->type = type;
	current->loc = loc;
	current->integer = 0;
	current->is_line_end = false;

	current->value = oak_malloc(end - start + 1);
	strncpy(current->value, start, end - start);
	current->value[end - start] = 0;

	current->next = NULL;
	current->prev = *prev;

	*prev = current;
	current = current->next;
}

char *token_get_str(enum token_type type)
{
	char *ret = NULL;

	if (type < TOK_INVALID) {
		ret = oak_malloc(strlen(token_type_str[type]) + 1);
		strcpy(ret, token_type_str[type]);
	} else {
		ret = oak_malloc(2);
		ret[1] = 0;
		ret[0] = (char)type;
	}

	return ret;
}

void token_write(struct token *tok, FILE *fp)
{
	fprintf(fp, "type       | data       | file       | index   | length | eol?  | value   \n");
	fprintf(fp, "--------------------------------------------------------------------------");

	while (tok) {
		fputc('\n', fp);
		if (tok->type <= TOK_INVALID)
			fprintf(fp, "%10s | ", token_type_str[(size_t)tok->type]);
		else
			fprintf(fp, "%10c | ", tok->type);

		switch (tok->type) {
		case TOK_FLOAT:
			fprintf(fp, "%10.4f | ", tok->floating);
			break;
		case TOK_INTEGER:
			fprintf(fp, "%10zd | ", tok->integer);
			break;
		case TOK_SYMBOL:
			fprintf(fp, "%10s | ", tok->value);
			break;
		case TOK_KEYWORD:
			fprintf(fp, "%10s | ", tok->keyword->body);
			break;
		default:
			fprintf(fp, "%10s | ", "");
		}

		fprintf(fp, "%10s | %7zd | ", tok->loc.file, tok->loc.index);
		fprintf(fp, "%6zd | %5s | %s",
			tok->loc.len, tok->is_line_end ? "true" : "false", tok->value);

		tok = tok->next;
	}
}
