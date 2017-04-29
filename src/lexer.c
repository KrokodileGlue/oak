#include "lexer.h"
#include "error.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

static char token_type[][64] = {
	"IDENTIFIER",
	"NUMBER    ",
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
			case '\n': continue; break; /* ignore */

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
			default:
				push_error((struct Location){loc.text, loc.file, loc.index + i}, ERR_FATAL, "unrecognized escape sequence");
				continue;
				break;
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

/*
 * Whenever we have two adjacent string tokens like "foo" "bar",
 * they ought to be combined into a new token that has
 * the data fields of the leftmost token, but which has
 * a body composed of both strings concatenated ("foobar").
 *
 * This function is recursive, and does multiple passes.
 *
 * Returns true if it found any strings to concatenate,
 * otherwise returns false.
 */
static bool concatenate_strings(struct Token* tok)
{
	bool ret = false;

	while (tok && tok->next) {
		if (tok->type == TOK_STRING && tok->next->type == TOK_STRING) {
			ret = true;
			size_t end = strlen(tok->value) + strlen(tok->next->value);
			tok->value = realloc(tok->value, strlen(tok->value) + strlen(tok->next->value) + 1);
			strcpy(tok->value + strlen(tok->value), tok->next->value);
			tok->value[end] = 0;

			/* now delete the token */
			/* TODO: make this into a function e.g. delete_token() */
			struct Token* third = tok->next->next;
			if (third) third->prev = tok;
			free(tok->next->value);
			free(tok->next);
			tok->next = third;
		}
		tok = tok->next;
	}

	/* rewind the token stream */
	if (tok)
		while (tok->prev) tok = tok->prev;
	
	if (ret) concatenate_strings(tok);

	return ret;
}

struct Token* tokenize(char* code, char* filename)
{
	struct Token *tok = NULL;
	char* ch = code;
	
	while (*ch) {
		struct Location loc = (struct Location){code, filename, ch - code};

		if (is_whitespace(*ch)) {
			while (is_whitespace(*ch) && *ch) {
				ch++;
			}
			continue;
		}

		if (!strncmp(ch, "/*", 2)) {
			ch += 2;
			char* end = ch;

			while ((strncmp(end, "*/", 2) && strncmp(end, "/*", 2)) && *end) end++;

			if (*end == 0 || !strncmp(end, "/*", 2)) {
				push_error(loc, ERR_FATAL, "unterminated comment");
				while (*ch != '\n' && *ch) ch++;
			}
			else ch = end + 2;
			continue;
		}

		if (!strncmp(ch, "//", 2)) {
			while (*ch != '\n' && *ch) ch++;
			continue;
		}

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
	concatenate_strings(tok);

	return tok;
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
		fprintf(fp, "[%s][%s]\n", token_type[tok->type], tok->value);
		tok = tok->next;
	}
}
