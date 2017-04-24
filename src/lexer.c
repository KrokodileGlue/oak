#include "lexer.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

char* lexer_error_str = NULL;
static void lexer_push_error(const char* text, size_t location, const char* str)
{
	if (!lexer_error_str) {
		lexer_error_str = malloc(1);
		*lexer_error_str = 0;
	}

	printf("%s: %s", text + location, str);
	lexer_error_str = realloc(lexer_error_str, strlen(lexer_error_str) + strlen(str) + 1);

	strcpy(lexer_error_str + strlen(str), str);
}

char* lexer_error()
{
	return lexer_error_str;
}

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

#define is_whitespace(is_whitespace_c) \
        (is_whitespace_c == ' ' || is_whitespace_c == '\t' || is_whitespace_c == '\n')

#define is_identifier_start(is_identifier_c) \
        ((is_identifier_c >= 'a' && is_identifier_c <= 'z') || (is_identifier_c >= 'A' && is_identifier_c <= 'Z') || is_identifier_c == '_')

#define is_legal_in_identifier(is_legal_in_identifier_c) \
        ((is_legal_in_identifier_c >= 'a' && is_legal_in_identifier_c <= 'z') \
        || (is_legal_in_identifier_c >= 'A' && is_legal_in_identifier_c <= 'Z') \
        || is_legal_in_identifier_c == '_' \
        || (is_legal_in_identifier_c >= '0' && is_legal_in_identifier_c <= '9'))

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

struct Token* lexer_tokenize(char* in)
{
	struct Token *prev = NULL;

	char *start = in, *end = in;
	size_t origin = 0;

	lexer_push_error(in, 0, "deadbeef");
	
	while (*start) {
		end = start;
		origin = start - in;
		
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
