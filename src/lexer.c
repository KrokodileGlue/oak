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
	free(ls);
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

#define SKIP_TO_END_OF_LINE(start, stop)	\
	stop = start;				\
	while (*stop != '\n' && *stop) { stop++; }

static void lexer_push_error(struct LexState *ls, enum ErrorLevel sev, char *fmt, ...)
{
	char* msg = oak_malloc(ERR_MAX_MESSAGE_LEN + 1);

	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, ERR_MAX_MESSAGE_LEN, fmt, args);
	va_end(args);

	error_push(ls->es, ls->loc, sev, msg);

	free(msg);
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
		ls->tok->integer = (int64_t)strtol(ls->tok->value, NULL, 16);
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
			ls->tok->integer = (int64_t)strtol(ls->tok->value, NULL, 0);
		} else {
			lexer_push_token(ls, TOK_FLOAT, a, b);
			ls->tok->floating = atof(ls->tok->value);
		}
	}

	return b;
}

static char *parse_string_literal(struct LexState *ls, char *a)
{
	char *b = a;
	do {
		if (*b == '\\') b++;
		if (*b) b++; /* don't skip over the null terminator */
	} while (*b != '\"' && *b != '\n' && *b);

	if (*b == 0 || *b == '\n') {
		SKIP_TO_END_OF_LINE(a, b);

		ls->loc.len = b - a + 1;
		lexer_push_error(ls, ERR_FATAL, "unterminated string literal");

		return b;
	}

	ls->loc.len = b - a + 1;
	lexer_push_token(ls, TOK_STRING, a, b + 1);

	ls->tok->string = oak_malloc(strlen(ls->tok->value) + 1);
	strncpy(ls->tok->string, ls->tok->value + 1, strlen(ls->tok->value));
	ls->tok->string[strlen(ls->tok->value) - 2] = 0; /* cut off the " at the end */

	parse_escape_sequences(ls, ls->tok->string);

	return b + 1;
}

static char *parse_character_literal(struct LexState *ls, char *a)
{
	char *b = a;
	do {
		if (*b == '\\') b++;
		if (*b) b++; /* don't skip over the null terminator */
	} while (*b != '\'' && *b != '\n' && *b);

	if (*b == 0 || *b == '\n') {
		SKIP_TO_END_OF_LINE(a, b);

		ls->loc.len = b - a + 1;
		lexer_push_error(ls, ERR_FATAL, "unterminated character literal");

		return b;
	}

	ls->loc.len = b - a;
	lexer_push_token(ls, TOK_STRING, a, b + 1);

	ls->tok->string = oak_malloc(strlen(ls->tok->value) + 2);

	strncpy(ls->tok->string, ls->tok->value + 1, strlen(ls->tok->value));
	ls->tok->string[strlen(ls->tok->value) - 2] = 0;

	parse_escape_sequences(ls, ls->tok->string);

	if (strlen(ls->tok->string) != 1) {
		/* add 2 to the length because the body of the token doesn't include the ' characters */
		ls->loc.len = b - a + 1;

		lexer_push_error(ls, ERR_WARNING, "multi-element character literal will be truncated to the first element");
	}

	int64_t t = (int64_t)(ls->tok->string[0]);
	free(ls->tok->string);

	ls->tok->type = TOK_INTEGER;
	ls->tok->integer = t;

	return b + 1;
}

static char *parse_raw_string_literal(struct LexState *ls, char *a)
{
	char *begin = a;
	a += 2; /* skip the R( */
	char *b = a;

	while (*b != ')' && *b) b++; /* find the end of the delimiter spec */

	if (*b == 0) {
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
		free(delim);

		return b;
	}

	ls->loc.len = strlen(delim) * 2 + 3 + (b - a);
	lexer_push_token(ls, TOK_STRING, begin, b + strlen(delim));

	ls->tok->string = oak_malloc(strlen(delim) + 4 + (b - a));
	strncpy(ls->tok->string, ls->tok->value + strlen(delim) + 3, (b - begin) - strlen(delim));
	ls->tok->string[b - a] = 0; /* cut off the delimiter at the end */

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

/* recursively concatenate adjacent strings */
static bool cat_strings(struct Token *tok)
{
	bool ret = false;
	token_rewind(&tok);

	while (tok && tok->next) {
		if (tok->type == TOK_STRING && tok->next->type == TOK_STRING) {
			ret = true;
			tok->string = smart_cat(tok->string, tok->next->string);
			tok->value = smart_cat(tok->value, tok->next->value);
			token_delete(tok->next);
		}
		tok = tok->next;
	}

	if (ret) cat_strings(tok);
	return ret;
}

/* match the longest operator starting from a */
static struct Operator *match_operator(char *a)
{
	size_t len = 0;
	struct Operator *op = NULL;

	for (size_t i = 0; i < num_ops(); i++) {
		if (!strncmp(ops[i].body, a, strlen(ops[i].body))
		    && strlen(ops[i].body) > len) {
			len = strlen(ops[i].body);
			op = ops + i;
		}
	}

	return op;
}

static char *parse_operator(struct LexState *ls, char *a)
{
	struct Operator *op = match_operator(a);
	size_t len = strlen(op->body);
	char *b = a + len;

	ls->loc.len = len;
	lexer_push_token(ls, TOK_SYMBOL, a, b);

	return b;
}

static char *parse_secondary_operator(char *a)
{
	size_t len = 0;

	for (size_t i = 0; i < num_ops(); i++) {
		if (!strncmp(ops[i].body2, a, strlen(ops[i].body2))
		    && strlen(ops[i].body2) > len) {
			len = strlen(ops[i].body2);
		}
	}

	return len ? a + len : 0;
}

struct Token *tokenize(struct LexState *ls)
{
	char *a = ls->text;

	while (*a) {
		ls->loc = (struct Location){ls->text, ls->file, a - ls->text, 1};

		if (is_whitespace(*a)) {
			while (is_whitespace(*a) && *a) {
				if (*a == '\n' && ls->tok) ls->tok->is_line_end = true;
				a++;
			}
			continue;
		}

		if (!strncmp(a, "/*", 2)) {
			char *b = a;
			size_t depth = 0;

			do {
				if (!strncmp(b, "/*", 2)) depth++;
				if (!strncmp(b, "*/", 2)) depth--;
				b++;
			} while (depth && *b);

			if (*b == 0) {
				ls->loc.len = 2;
				lexer_push_error(ls, ERR_FATAL, "unmatched comment initializer");

				while (*a != '\n' && *a) a++;
			} else {
				a = b + 1;
			}

			continue;
		}

		if (!strncmp(a, "*/", 2)) {
			ls->loc.len = 2;
			lexer_push_error(ls, ERR_FATAL, "unmatched comment terminator");
			a += 2;

			continue;
		}

		if (!strncmp(a, "//", 2) || !strncmp(a, "#", 1)) {
			while (*a != '\n' && *a) a++;

			continue;
		}

		if (!strncmp(a, "R(", 2)) {
			a = parse_raw_string_literal(ls, a);
		} else if (match_operator(a)) {
			a = parse_operator(ls, a);
		} else if (is_identifier_start(*a)) {
			a = parse_identifier(ls, a);

			for (size_t i = 0; i < num_keywords(); i++) {
				if (!strcmp(ls->tok->value, keywords[i].body)) {
					ls->tok->type = TOK_KEYWORD;
					ls->tok->keyword = keywords + i;
					break;
				}
			}

			if (!strcmp(ls->tok->value, "true") || !strcmp(ls->tok->value, "false")) {
				ls->tok->type = TOK_BOOL;
				ls->tok->boolean = strcmp(ls->tok->value, "true") ? false : true;
			}
		} else if (*a == '\"') {
			a = parse_string_literal(ls, a);
		} else if (is_dec_digit(*a) || *a == '.') {
			a = parse_number(ls, a);
		} else if (*a == '\'') {
			a = parse_character_literal(ls, a);
		} else if (parse_secondary_operator(a)) {
			char *b = parse_secondary_operator(a);

			ls->loc.len = b - a;
			lexer_push_token(ls, TOK_SYMBOL, a, b);

			a = b;
		} else {
			ls->loc.len = 1;
			lexer_push_token(ls, TOK_SYMBOL, a, a + 1);
			a++;
		}
	}

	if (ls->tok) ls->tok->is_line_end = true;
	lexer_push_token(ls, TOK_END, a, a);
	cat_strings(ls->tok);
	token_rewind(&ls->tok);

	return ls->tok;
}
