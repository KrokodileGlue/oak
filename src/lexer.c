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

#define is_identifier_start(c)                                        \
        ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')

#define is_legal_in_identifier(c) \
        ((c >= 'a' && c <= 'z')   \
        || (c >= 'A' && c <= 'Z') \
        || c == '_'               \
        || (c >= '0' && c <= '9'))

#define is_hex_char(c)            \
        ((c >= 'a' && c <= 'z')   \
        || (c >= 'A' && c <= 'Z'))\
	|| (c >= '0' && c <= '9')

#define is_oct_char(c)        \
	(c >= '0' && c <= '7')

static void make_token(int type, size_t origin, char* start, char* end, struct Token** prev)
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

#define MAX_HEX_ESCAPE_SEQUENCE_LEN 64
#define MAX_OCT_ESCAPE_SEQUENCE_LEN 64

static void parse_escape_sequences(const char* code, struct Token* tok, char* str)
{
	char* out = str;
	size_t i = 0;
	do {
		char a = str[i];

		if (a == '\\') {
			a = str[++i];
			switch (a) {
			case 'a': a = '\a'; break;
			case 'b': a = '\b'; break;
			case 'f': a = '\f'; break;
			case 'n': a = '\n'; break;
			case 'r': a = '\r'; break;
			case 't': a = '\t'; break;
			case 'v': a = '\v'; break;
			case '\\': a = '\\'; break;
			case '\'': a = '\''; break;
			case '"': a = '"'; break;
			case '?': a = '?'; break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7': {
				size_t j = 0;
				char octal_sequence[MAX_OCT_ESCAPE_SEQUENCE_LEN + 1]; /* a string */

				a = str[++i];
				while (is_oct_char(a) && a && j <= MAX_OCT_ESCAPE_SEQUENCE_LEN) {
					octal_sequence[j++] = a;
					a = str[++i];
				}
				
				octal_sequence[j] = 0;
				printf("found octal escape sequence: %s\n", octal_sequence);
				a = (char)strtol(octal_sequence, NULL, 8);
			} break;
			case 'x': {
				size_t j = 0;
				char hex_sequence[MAX_HEX_ESCAPE_SEQUENCE_LEN + 1]; /* a string */

				a = str[++i];
				while (is_hex_char(a) && a && j <= MAX_HEX_ESCAPE_SEQUENCE_LEN) {
					hex_sequence[j++] = a;
					a = str[++i];
				}
				
				hex_sequence[j] = 0;
				printf("found hex escape sequence: %s\n", hex_sequence);
				a = (char)strtol(hex_sequence, NULL, 16);
			} break;
			default: push_error(code, tok->origin, "unrecognized escape sequence"); break;
			}
		}
		
		*out++ = a;
	} while (str[i++]);
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
			make_token(TOK_IDENTIFIER, origin, start, end, &prev);
			start = end;
		} else if (*start == '\"') {
			start++;
			do {
				if (*end == '\\') end++;
				end++;
			} while (*end != '\"' && *end);
			make_token(TOK_STRING, origin, start, end, &prev);
			parse_escape_sequences(str, prev, prev->value);
			end++;
			start = end;
		} else {
			end++;
			make_token(TOK_SYMBOL, origin, start, end, &prev);
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
