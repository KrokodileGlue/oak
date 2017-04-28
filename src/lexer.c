#include "lexer.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

char token_type[][64] = {
	"IDENTIFIER",
	"INTEGER   ",
	"STRING    ",
	"OPERATOR  ",
	"SYMBOL    ",
	"INVALID   "
};

static char operators[][3] = {
	"+",  "-",  "*",  "/",
	"+=", "-=", "*=", "/="

	">",  "<",
	">=", "<=", "=="

	"&",  "|",  "^",  "~",
	"&=", "|=", "^=", "~=",

	"&&", "||",

	">>", "<<", ">>=", "<<="
};

#define is_whitespace(c) \
        (c == ' ' || c == '\t' || c == '\n')

#define is_identifier_start(c) \
        ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')

#define is_legal_in_identifier(c) \
        ((c >= 'a' && c <= 'z') \
        || (c >= 'A' && c <= 'Z') \
        || c == '_' \
        || (c >= '0' && c <= '9'))

static void construct_token(int type, size_t origin, char* start, char* end, struct Token** prev)
{
	/* type, origin, data, value, next, prev */
	struct Token* current = malloc(sizeof (struct Token));
	if (*prev) (*prev)->next = current;

	current->type = type;
	current->origin = origin;
	current->fData = 0.05;

	current->value = malloc(end - start + 1);
	strncpy(current->value, start, end - start);
	current->value[end - start] = 0;

	current->next = NULL;
	current->prev = *prev;

	*prev = current;
	current = current->next;
}

struct Token* lexer_tokenize(char* str)
{
	struct Token *prev = NULL;

	char *start = str, *end = str;
	size_t origin = 0;

	push_error(str, "deadbeef", 0);
	push_error(str, "other error", 5);
	push_error(str, "bad thing", 15);
	
	while (*start) {
		end = start;
		origin = start - str;
		
		if (is_whitespace(*start)) {
			while (is_whitespace(*start) && *start) {
				start++;
			}
			continue;
		}

		if (is_identifier_start(*start)) {
			while (is_legal_in_identifier(*end) && *end) end++;
			construct_token(TOK_IDENTIFIER, origin, start, end, &prev);
			start = end;
		} else if (*start == '\"') {
			start++;
			do { end++; } while (*end != '\"' && *end);
			construct_token(TOK_STRING, origin, start, end, &prev);
			end++;
			start = end;
		} else {
			end++;
			construct_token(TOK_SYMBOL, origin, start, end, &prev);
			start = end;
		}
	}

	struct Token* head = prev;
	if (head) while (head->prev) head = head->prev;

	return head;
}

char* lexer_dump(struct Token* tok)
{
	while (tok) {
		printf("[%s][%s]\n", token_type[tok->type], tok->value);
		tok = tok->next;
	}

	return "";
}
