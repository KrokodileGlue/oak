#include "lexer.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

static char token_type[][64] = {
	"IDENTIFIER",
	"INTEGER   ",
	"STRING    ",
	"OPERATOR  ",
	"SYMBOL    ",
	"INVALID   "
};

static char operators[][64] = {
	"+",  "-",  "*",  "/",
	"+=", "-=", "*=", "/=",

	">",  "<",
	">=", "<=", "==",

	"&",  "|",  "^",  "~",
	"&=", "|=", "^=", "~=",

	"&&", "||",

	">>", "<<", ">>=", "<<="
};

static inline bool is_whitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n');
}

static inline bool is_identifier_start(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_');
}

static inline bool is_legal_in_identifier(char c)
{
	return ((c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| c == '_'
		|| (c >= '0' && c <= '9'));
}

static inline bool is_hex_digit(char c)
{
	return ((c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z'))
		|| (c >= '0' && c <= '9');
}

static inline bool is_oct_digit(char c)
{
	return (c >= '0' && c <= '7');
}

static void push_token(struct Location loc, int type, char* start, char* end, struct Token** prev)
{
	/* type, origin, data, value, next, prev */
	struct Token* current = malloc(sizeof (struct Token));
	if (*prev) (*prev)->next = current;

	current->type = type;
	current->loc = loc;
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

static void parse_escape_sequences(struct Location loc, char* str)
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

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7': {
				size_t j = 0;
				char oct_sequence[MAX_OCT_ESCAPE_SEQUENCE_LEN + 1]; /* a string */

				a = str[++i];
				while (is_oct_digit(a) && a && j <= MAX_OCT_ESCAPE_SEQUENCE_LEN) {
					oct_sequence[j++] = a;
					a = str[++i];
				}
				
				oct_sequence[j] = 0;
				//printf("found octal escape sequence: %s\n", oct_sequence);
				a = (char)strtol(oct_sequence, NULL, 8);
			} break;
			case 'x': {
				size_t j = 0;
				char hex_sequence[MAX_HEX_ESCAPE_SEQUENCE_LEN + 1]; /* a string */

				a = str[++i];
				while (is_hex_digit(a) && a && j <= MAX_HEX_ESCAPE_SEQUENCE_LEN) {
					hex_sequence[j++] = a;
					a = str[++i];
				}
				
				hex_sequence[j] = 0;
				//printf("found hex escape sequence: %s\n", hex_sequence);
				a = (char)strtol(hex_sequence, NULL, 16);
			} break;
			default: push_error((struct Location){loc.text, loc.file, loc.index + i}, ERR_FATAL, "unrecognized escape sequence"); break;
			}
		}
		
		*out++ = a;
	} while (str[i++]);
}

static char* parse_identifier(char* ch, struct Location loc, struct Token** tok)
{
	char* end = ch;
	while (is_legal_in_identifier(*end) && *end) end++;

	push_token(loc, TOK_IDENTIFIER, ch, end, tok);

	return end;
}

static char* parse_string_literal(char* ch, struct Location loc, struct Token** tok)
{
	char* end = ch++;
	do {
		if (*end == '\\') end++;
		end++;
	} while (*end != '\"' && *end);

	push_token(loc, TOK_STRING, ch, end, tok);
	parse_escape_sequences(loc, (*tok)->value);

	return end + 1;
}

static char* parse_raw_string_literal(char* ch, struct Location loc, struct Token** tok)
{
	ch += 2; /* skip the R( */
	char* end = ch;
	while (*end != ')' && *end) end++;
	if (*end == 0) {
		push_error(loc, ERR_FATAL, "unterminated delimiter specification");
		return end;
	}

	char* delim = malloc(end - ch + 1);
	strncpy(delim, ch, end - ch);
	delim[end - ch] = 0;
	size_t delim_len = strlen(delim);

	end++; /* skip the ) */
	ch = end;
	while (strncmp(end, delim, delim_len) && *end) end++;

	if (*end == 0) {
		push_error(loc, ERR_FATAL, "unterminated raw string literal");
		return end;
	}

	push_token(loc, TOK_STRING, ch, end, tok);
	free(delim);
	
	return end + delim_len;
}

static char* skip_comments_and_whitspace(char* ch)
{
	while (true) {
		if (is_whitespace(*ch)) {
			while (is_whitespace(*ch) && *ch) {
				ch++;
			}
			continue;
		}

		if (!strncmp(ch, "/*", 2)) {
			while (strncmp(ch, "*/", 2) && *ch) ch++;
			ch += 2;
			continue;
		}

		if (!strncmp(ch, "//", 2)) {
			while (*ch != '\n' && *ch) ch++;
			continue;
		}

		break;
	}

	return ch;
}

struct Token* tokenize(char* code, char* filename)
{
	struct Token *tok = NULL;
	char* ch = code;
	
	while (*(ch = skip_comments_and_whitspace(ch))) {
		struct Location loc = (struct Location){code, filename, ch - code};
		if (!strncmp(ch, "R(", 2)) {
			ch = parse_raw_string_literal(ch, loc, &tok);
		} else if (is_identifier_start(*ch)) {
			ch = parse_identifier(ch, loc, &tok);
		} else if (*ch == '\"') {
			ch = parse_string_literal(ch, loc, &tok);
		} else {
			push_token(loc, TOK_SYMBOL, ch, ch + 1, &tok);
			ch++;
		}
	}

	/* rewind the token stream */
	if (tok)
		while (tok->prev) tok = tok->prev;

	return tok;
}

void write_tokens(FILE* fp, struct Token* tok)
{
	while (tok) {
		fprintf(fp, "[%s][%s]\n", token_type[tok->type], tok->value);
		tok = tok->next;
	}
}
