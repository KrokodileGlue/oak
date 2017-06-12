#include "lexer.h"
#include "util.h"
#include "keyword.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_HEX_ESCAPE_SEQUENCE_LEN 64
#define MAX_OCT_ESCAPE_SEQUENCE_LEN 64

void lexer_clear(struct LexState *ls)
{
	error_clear(ls->es);
}

struct LexState *lexer_new(char *text, char *file)
{
	struct LexState *ls = oak_malloc(sizeof *ls);

	ls->text = text;
	ls->file = file;
	ls->es = error_new();
	ls->tok = NULL;

	return ls;
}

#define SKIP_TO_END_OF_LINE(start, stop)          \
	stop = start;                             \
	while (*stop != '\n' && *stop) { stop++; }

static void lexer_push_error(struct LexState *ls, enum ErrorLevel sev, char *fmt, ...)
{
       	char* msg = oak_malloc(ERR_MAX_MESSAGE_LEN + 1);

	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, ERR_MAX_MESSAGE_LEN, fmt, args);
	va_end(args);
	
	error_push(ls->es, ls->loc, sev, msg);
}

static void lexer_push_token(struct LexState *ls, enum TokType type, char *a, char *b)
{
	token_push(ls->loc, type, a, b, &ls->tok);
}

static void parse_escape_sequences(struct LexState *ls, char *str)
{
	char *out = str;
	size_t i = 0;
	do {
		char a = str[i];
		//ls->loc.index += 1;
		
		if (a == '\\') {
			i++;
			a = str[i];
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
			case '\n': continue; /* ignore */

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7': {
				size_t j = 0;
				char oct_sequence[MAX_OCT_ESCAPE_SEQUENCE_LEN + 1]; /* a string */

				a = str[i];
				while (is_oct_digit(a) && a && j <= MAX_OCT_ESCAPE_SEQUENCE_LEN) {
					oct_sequence[j++] = a;
					a = str[++i];
				}
				i--;
				
				oct_sequence[j] = 0;
				//printf("found octal escape sequence: %s\n", oct_sequence);

				long int data = strtol(oct_sequence, NULL, 8);
				if (data > 0xFF) {
					ls->loc.index += i - j + 1;
					ls->loc.len = j + 1; /* add 1 for the \ */
					lexer_push_error(ls, ERR_WARNING, "octal value %ld is too large and will be masked to the lowest byte", data);
					ls->loc.index -= i - j + 1;
				}

				a = (char)(data & 0xFF);
			} break;
			case 'x': {
				size_t j = 0;
				char hex_sequence[MAX_HEX_ESCAPE_SEQUENCE_LEN + 1]; /* a string */

				a = str[++i];
				while (is_hex_digit(a) && a && j <= MAX_HEX_ESCAPE_SEQUENCE_LEN) {
					hex_sequence[j++] = a;
					a = str[++i];
				}
				i--;
				
				hex_sequence[j] = 0;
				//printf("found hex escape sequence: %s\n", hex_sequence);

				long int data = strtol(hex_sequence, NULL, 16);
				if (data > 0xFF) {
					ls->loc.index += i - j;
					ls->loc.len = j + 2; /* add two for the \x */
					lexer_push_error(ls, ERR_WARNING, "hexadecimal value %ld is too large and will be masked to the lowest byte", data);
					ls->loc.index -= i - j;
				}

				a = (char)(data & 0xFF);
			} break;
			default: {
				ls->loc.index += i;
				ls->loc.len = 2;
				lexer_push_error(ls, ERR_WARNING, "unrecognized escape sequence: '\\%c'", a);
				ls->loc.index -= i;
				continue;
			}
			}
		}
		
		*out++ = a;
	} while (str[i++]);
}

static char *parse_identifier(struct LexState *ls, char *a)
{
	char *b = a;
	while (is_legal_in_identifier(*b) && *b) b++;

	ls->loc.len = b - a;
	lexer_push_token(ls, TOK_IDENTIFIER, a, b);

	return b;
}

static char *parse_number(struct LexState *ls, char *a)
{
	char *b = a;

	if (!strncmp(a, "0x", 2)) {
		b += 2;
		while (is_hex_digit(*b)) b++;

		ls->loc.len = b - a;
		lexer_push_token(ls, TOK_INTEGER, a, b);
		ls->tok->iData = (int64_t)strtol(ls->tok->value, NULL, 16);
	} else {
		bool is_integer = true;
		bool is_oct = (*a == '0'), found_exponent = false;
		
		while (true) {
			if (*b == 'e') {
				if (found_exponent || is_integer) {
					ls->loc.len = b - a;
					lexer_push_error(ls, ERR_FATAL, "incorrect use of scientific notation in numeric literal");
					break;
				}

				found_exponent = true;
				b++;
				if (*b == '-' || *b == '+') b++;
				continue;
			}
			
			if ((is_oct && is_oct_digit(*b)) || is_dec_digit(*b)) {
				b++;
				continue;
			}

			if (*b == '.') {
				if (is_integer) {
					is_integer = false;
				} else {
					/* we've run into a second . for this number, e.g. 0.1.2
					 * we will break, so that example will be parsed as 0.1, 0.2 */
					break;
				}
				
				b++;
				continue;
			}

			break;
		}

		ls->loc.len = b - a;
		if (is_integer) {
			lexer_push_token(ls, TOK_INTEGER, a, b);
			ls->tok->iData = (int64_t)strtol(ls->tok->value, NULL, 0);
		} else {
			lexer_push_token(ls, TOK_FLOAT, a, b);
			ls->tok->fData = atof(ls->tok->value);
		}
	}

	return b;
}

static char *parse_string_literal(struct LexState *ls, char *a)
{
	a += 1;
	char *b = a;
	do {
		if (*b == '\\') b++;
		b++;
	} while (*b != '\"' && *b != '\n' && *b);

	if (*b == 0 || *b == '\n') {
		SKIP_TO_END_OF_LINE(a, b);

		ls->loc.len = b - a + 1;
		lexer_push_error(ls, ERR_FATAL, "unterminated string literal");
		
		return b;
	}

	ls->loc.len = b - a;
	lexer_push_token(ls, TOK_STRING, a, b);
	parse_escape_sequences(ls, ls->tok->value);

	return b + 1;
}

static char *parse_character_literal(struct LexState *ls, char *a)
{
	a += 1; /* skip the ' */
	char *b = a;
	do {
		if (*b == '\\') b++;
		if (*b) b++; /* we might skip over the end of the string without the conditional */
	} while (*b != '\'' && *b != '\n' && *b);

	if (*b == 0 || *b == '\n') {
		SKIP_TO_END_OF_LINE(a, b);

		ls->loc.len = b - a + 1;
		lexer_push_error(ls, ERR_FATAL, "unterminated character literal");
		
		return b;
	}

	ls->loc.len = b - a;
	lexer_push_token(ls, TOK_STRING, a, b);
	parse_escape_sequences(ls, ls->tok->value);

	if (strlen(ls->tok->value) != 1) {
		/* add 2 to the length because the body of the token doesn't include the ' characters */
		ls->loc.len = b - a + 2;

		lexer_push_error(ls, ERR_WARNING, "multi-element character literal will be truncated to the first element");
	}

	ls->tok->type = TOK_INTEGER;
	ls->tok->iData = (int64_t)ls->tok->value[0];

	return b + 1;
}

static char *parse_raw_string_literal(struct LexState *ls, char *a)
{
	a += 2; /* skip the R( */
	char *b = a;

	while (*b != ')' && *b) b++; /* find the end of the delimiter spec */

	if (*b == 0) {
		SKIP_TO_END_OF_LINE(a, b)

		ls->loc.len = b - a;
		lexer_push_error(ls, ERR_FATAL, "unterminated delimiter specification");

		return b;
	}

	char *delim = oak_malloc(b - a + 1);
	strncpy(delim, a, b - a);
	delim[b - a] = 0;
	size_t delim_len = strlen(delim);

	b++; /* skip the ) */
	a = b;
	/* find the end of the string with the delimiter */
	while (strncmp(b, delim, delim_len) && *b) b++;

	if (*b == 0) {
		ls->loc.len = strlen(delim) + 3;
		lexer_push_error(ls, ERR_FATAL, "unterminated raw string literal");

		SKIP_TO_END_OF_LINE(a, b)
		free(delim);

		return b;
	}

	ls->loc.len = strlen(delim) + 4 + (b - a);
	lexer_push_token(ls, TOK_STRING, a, b);
	free(delim);
	
	return b + delim_len;
}

static char *smart_cat(char *first, char *second)
{
	size_t len = strlen(first) + strlen(second);
	
	first = realloc(first, len + 1);
	strcpy(first + strlen(first), second);

	first[len] = 0;

	return first;
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
static bool cat_strings(struct Token *tok)
{
	bool ret = false;
	token_rewind(&tok);
	
	while (tok && tok->next) {
		if (tok->type == TOK_STRING && tok->next->type == TOK_STRING) {
			ret = true;
			tok->value = smart_cat(tok->value, tok->next->value);
			token_delete(tok->next);
		}
		tok = tok->next;
	}

	if (ret) cat_strings(tok);
	return ret;
}

static char *parse_operator(struct LexState *ls, char *a)
{
	enum OpType op_type = match_operator(a);
	size_t len = get_op_len(op_type);
	char *b = a + len;

	ls->loc.len = len;
	lexer_push_token(ls, TOK_OPERATOR, a, b);
	ls->tok->op_type = op_type;

	return b;
}

struct Token *tokenize(struct LexState *ls)
{
	char *a = ls->text;
	
	while (*a) {
		ls->loc = (struct Location){ls->text, ls->file, a - ls->text, 1};
		
		if (is_whitespace(*a)) {
			while (is_whitespace(*a) && *a) {
				a++;
			}
			continue;
		}

		if (!strncmp(a, "/*", 2)) {
			a += 2;
			char *b = a;

			while (strncmp(b, "*/", 2) && *b) b++;

			if (*b == 0) {
				ls->loc.len = 2;
				lexer_push_error(ls, ERR_FATAL, "unmatched comment initializer");

				while (*a != '\n' && *a) a++;
			} else {
				a = b + 2;
			}
			
			continue;
		}

		if (!strncmp(a, "*/", 2)) {
			ls->loc.len = 2;
			lexer_push_error(ls, ERR_FATAL, "unmatched comment terminator");
			a += 2;

			continue;
		}

		if (!strncmp(a, "//", 2)) {
			while (*a != '\n' && *a) a++;

			continue;
		}

		if (!strncmp(a, "R(", 2)) {
			a = parse_raw_string_literal(ls, a);
		} else if (is_identifier_start(*a)) {
			a = parse_identifier(ls, a);
			
			if (keyword_get_type(ls->tok->value) != KEYWORD_INVALID) {
				ls->tok->type = TOK_KEYWORD;
				ls->tok->keyword_type = keyword_get_type(ls->tok->value);
			}

			if (!strcmp(ls->tok->value, "true") || !strcmp(ls->tok->value, "false")) {
				ls->tok->type = TOK_BOOL;
				ls->tok->bool_type = strcmp(ls->tok->value, "true") ? false : true;
			}
		} else if (*a == '\"') {
			a = parse_string_literal(ls, a);
		} else if (is_dec_digit(*a) || *a == '.') {
			a = parse_number(ls, a);
		} else if (match_operator(a) != OP_INVALID) {
			a = parse_operator(ls, a);
		} else if (*a == '\'') {
			a = parse_character_literal(ls, a);
		} else {
			ls->loc.len = 1;
			lexer_push_token(ls, (enum TokType)*a, a, a + 1);
			a++;
		}
	}

	lexer_push_token(ls, TOK_END, a, a);
	cat_strings(ls->tok);
	token_rewind(&ls->tok);

	return ls->tok;
}
