#include "lexer.h"
#include "util.h"
#include "keyword.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void
free_lexer(struct lexer *ls)
{
	error_clear(ls->r);
	free(ls);
}

struct lexer *
new_lexer(char *text, char *file)
{
	struct lexer *ls = oak_malloc(sizeof *ls);

	ls->text = text;
	ls->file = file;
	ls->r = new_reporter();
	ls->tok = NULL;

	return ls;
}

#define SKIP_TO_END_OF_LINE(start, stop)	\
	stop = start;				\
	while (*stop != '\n' && *stop) { stop++; }

static void
lexer_push_error(struct lexer *ls, enum error_level sev, char *fmt, ...)
{
	char* msg = oak_malloc(ERR_MAX_MESSAGE_LEN + 1);

	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, ERR_MAX_MESSAGE_LEN, fmt, args);
	va_end(args);

	error_push(ls->r, ls->loc, sev, msg);

	free(msg);
}

static void
lexer_push_token(struct lexer *ls, enum token_type type, char *a, char *b)
{
	token_push(ls->loc, type, a, b, &ls->tok);
}

static void
parse_escape_sequences(struct lexer *ls, char *str)
{
	char *out = str;
	size_t i = 0;
	do {
		if (str[i] == '\\' && str[i + 1] == '{') {
			*out++ = str[i++];
			*out++ = str[i];
			continue;
		}

		char a = str[i];

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
				int data = 0;
				size_t j = 0;

				while (is_oct_digit(a) && a) {
					data *= 8;
					data += a - '0';
					a = str[++i];
					j++;
				}

				if (data > 0xFF) {
					ls->loc.index += i - j + 1;
					ls->loc.len = j + 1; /* add 1 for the \ */
					lexer_push_error(ls, ERR_WARNING, "value %ld is too large", data);
					ls->loc.index -= i - j + 1;
				}

				a = (char)data;
			} break;
			case 'x': {
				int data = 0;
				size_t j = 0;

				while (is_hex_digit(a) && a) {
					data *= 16;
					data += a - '0';
					a = str[++i];
					j++;
				}

				if (data > 0xFF) {
					ls->loc.index += i - j + 1;
					ls->loc.len = j + 1; /* add 1 for the \ */
					lexer_push_error(ls, ERR_WARNING, "value %ld is too large", data);
					ls->loc.index -= i - j + 1;
				}

				a = (char)data;
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

static char *
parse_identifier(struct lexer *ls, char *a)
{
	char *b = a;
	while (is_legal_in_identifier(*b) && *b) {
		if (*b == '-' && b[1] == '-') break;
		b++;
	}

	ls->loc.len = b - a;
	lexer_push_token(ls, TOK_IDENTIFIER, a, b);

	return b;
}

static char *
parse_number(struct lexer *ls, char *a)
{
	char *b = a;

	if (!strncmp(a, "0x", 2)) {
		b += 2;
		while (is_hex_digit(*b)) b++;

		ls->loc.len = b - a;
		lexer_push_token(ls, TOK_INTEGER, a, b);
		ls->tok->integer = (int64_t)strtol(ls->tok->value, NULL, 16);

		return b;
	}

	bool is_integer = true;
	bool is_oct = (*a == '0'), found_exponent = false;

	while (true) {
		if (*b == 'e') {
			if (found_exponent || is_integer) {
				ls->loc.len = b - a;
				lexer_push_error(ls, ERR_FATAL, "cannot use scientific notation in floating point literals");
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
				/*
				 * We've run into a second . for this
				 * number, e.g. 0.1.2 we will break,
				 * so that example will be parsed as
				 * 0.1, 0.2
				 */
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

	return b;
}

static char *
parse_string_literal(struct lexer *ls, char *a)
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
	ls->tok->is_interpolatable = false;

	ls->tok->string = oak_malloc(strlen(ls->tok->value) + 1);
	strncpy(ls->tok->string, ls->tok->value + 1, strlen(ls->tok->value));
	ls->tok->string[strlen(ls->tok->value) - 2] = 0; /* cut off the " at the end */

	parse_escape_sequences(ls, ls->tok->string);

	return b + 1;
}

static char *
parse_interpolated_string(struct lexer *ls, char *a)
{
	char *b = a;
	do {
		if (*b == '\\') b++;
		if (*b) b++; /* don't skip over the null terminator */
	} while (*b != '\'' && *b != '\n' && *b);

	if (*b == 0 || *b == '\n') {
		SKIP_TO_END_OF_LINE(a, b);

		ls->loc.len = b - a + 1;
		lexer_push_error(ls, ERR_FATAL, "unterminated string literal");

		return b;
	}

	ls->loc.len = b - a + 1;
	lexer_push_token(ls, TOK_STRING, a, b + 1);
	ls->tok->is_interpolatable = true;

	ls->tok->string = oak_malloc(strlen(ls->tok->value) + 1);
	strncpy(ls->tok->string, ls->tok->value + 1, strlen(ls->tok->value));
	ls->tok->string[strlen(ls->tok->value) - 2] = 0; /* cut off the ' at the end */

	parse_escape_sequences(ls, ls->tok->string);

	return b + 1;
}

static char *
parse_raw_string_literal(struct lexer *ls, char *a)
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

/* match the longest operator starting from `a` */
static struct operator *
match_operator(char *a)
{
	size_t len = 0;
	struct operator *op = NULL;

	for (size_t i = 0; i < num_ops(); i++) {
		if (!strncmp(ops[i].body, a, strlen(ops[i].body))
		    && strlen(ops[i].body) > len) {
			len = strlen(ops[i].body);
			op = ops + i;
		}
	}

	return op;
}

static char *
parse_operator(struct lexer *ls, char *a)
{
	struct operator *op = match_operator(a);
	size_t len = strlen(op->body);
	char *b = a + len;

	ls->loc.len = len;
	lexer_push_token(ls, TOK_SYMBOL, a, b);

	return b;
}

static char *
parse_secondary_operator(char *a)
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

static char *
parse_include(struct lexer *ls, char *a)
{
	char *begin = a;
	a += 2;
	char *b = a;

	while (*b != ')' && *b) b++;
	if (!*b) {
		ls->loc.len = b - a;
		lexer_push_error(ls, ERR_FATAL, "unterminated path");
		b = a;
		SKIP_TO_END_OF_LINE(a, b);
	}

	char *path = oak_malloc(b - a + 1);
	strncpy(path, a, b - a);
	path[b - a] = 0;
	b++;

	char *text = load_file(path);

	if (!text) {
		lexer_push_error(ls, ERR_FATAL, "could not load file");
		free(path);
		return b;
	}

	ls->loc.len = b - begin;
	lexer_push_token(ls, TOK_STRING, begin, b);
	ls->tok->string = text;
	free(path);

	return b;
}

static char *
parse_regex(struct lexer *ls, char *a)
{
	char delim = 0;
	char *b = a;

	enum {
		REGEX_SUBSTITUTION,
		REGEX_MATCH
	} type = REGEX_MATCH;

	/* TODO: qw// n stuff */
	if (*a == 's') {
		type = REGEX_SUBSTITUTION;
		b++; a = b;
	}

	delim = *b;
	b++;

	/*
	 * A regular expression cannot appear at the beginning
	 * of the program, so it's okay to look behind.
	 */
	while (*b && *b != '\n') {
		if (*b == delim) {
			if (!(b[-1] == '\\' && b[-2] != '\\'))
				break;
		}
		b++;
	}

	if (*b == '\n') {
		ls->loc.len = b - a + 2;
		lexer_push_error(ls, ERR_FATAL, "unterminated regular expression");
		return b;
	}

	b--;
	ls->loc.len = b - a + 2;
	lexer_push_token(ls, TOK_REGEX, a, b + 2);
	ls->tok->substitution = NULL;
	ls->tok->regex = oak_malloc(b - a + 1);
	strncpy(ls->tok->regex, a + 1, b - a);
	ls->tok->regex[b - a] = 0;
	b += 2;
	a = b;

	if (type == REGEX_SUBSTITUTION) {
		while (*b && *b != '\n') {
			if (*b == delim) {
				if (!(b[-1] == '\\' && b[-2] != '\\'))
					break;
			}
			b++;
		}

		ls->tok->substitution = oak_malloc(b - a + 1);
		strncpy(ls->tok->substitution, a, b - a);
		ls->tok->substitution[b - a] = 0;
		b++;
		a = b;
	}

	while (*b && isalpha(*b)) b++;

	ls->tok->flags = oak_malloc(b - a + 1);
	strncpy(ls->tok->flags, a, b - a);
	ls->tok->flags[b - a] = 0;
	ls->tok->loc.len += b - a;

	return b;
}

static char *
parse_group(struct lexer *ls, char *a)
{
	char *b = a + 1;
	while (*b && is_dec_digit(*b)) b++;

	if (b == a + 1) {
		ls->loc.len = b - a;
		lexer_push_error(ls, ERR_FATAL, "group reference requires number");
		SKIP_TO_END_OF_LINE(a, b);
		return b;
	}

	ls->loc.len = b - a;
	lexer_push_token(ls, TOK_GROUP, a, b);

	int i = 0;
	a++;
	while (is_dec_digit(*a)) {
		i *= 10;
		i += *a - '0';
		a++;
	}

	ls->tok->integer = i;
	return b;
}

static bool
is_regex_start(struct lexer *ls, char *a)
{
	static char *forbid = ",{}[]\'\"!:-+()><;*%=?";

	if (!*a)       return false;
	if (!ls->tok)  return false;
	if (!strcmp(ls->tok->value, ")")) return false;
	if (ls->tok->type == TOK_INTEGER) return false;
	if (ls->tok->type == TOK_FLOAT)   return false;
	if (ls->tok->type == TOK_BOOL)    return false;
	if (is_identifier_start(*a) && *a != 's') return false;

	if (ls->tok->type == TOK_IDENTIFIER) {
		/* Regular expressions may follow builtin functions and keywords. */
		bool pass = false;

		for (size_t i = 0; i < num_keywords(); i++)
			if (!strcmp(ls->tok->value, keywords[i].body)) {
				pass = true;
				break;
			}

		if (get_builtin(ls->tok->value)) pass = true;
		if (!pass) return false;
	}

	if (ispunct(*a) && !strchr(forbid, *a))                  return true;
	if (*a == 's' && ispunct(a[1]) && !strchr(forbid, a[1])) return true;

	return false;
}

bool
tokenize(struct module *m)
{
	struct lexer *ls = new_lexer(m->text, m->path);
	char *a = m->text;

	while (*a) {
		ls->loc = (struct location){ ls->text, ls->file, a - ls->text, 1 };

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

		if ((!strncmp(a, "//", 2) || !strncmp(a, "#", 1)) && (ls->tok ? strcmp(ls->tok->value, "split") : true)) {
			while (*a != '\n' && *a) a++;

			continue;
		}

		/* TODO: qw// n stuff */
		if (*a == '$') {
			a = parse_group(ls, a);
		} else if (!strncmp(a, "...", 3)) {
			size_t len = 3;
			char *b = a + len;
			ls->loc.len = len;
			lexer_push_token(ls, TOK_SYMBOL, a, b);
			a += 3;
		} else if (!strncmp(a, "=>", 2)) {
			size_t len = 2;
			char *b = a + len;
			ls->loc.len = len;
			lexer_push_token(ls, TOK_SYMBOL, a, b);
			a += 2;
		} else if (is_regex_start(ls, a)) {
			a = parse_regex(ls, a);
		} else if (!strncmp(a, "R(", 2)) {
			a = parse_raw_string_literal(ls, a);
		} else if (!strncmp(a, "I(", 2)) {
			a = parse_include(ls, a);
		} else if (match_operator(a)) {
			a = parse_operator(ls, a);
		} else if (is_identifier_start(*a)) {
			a = parse_identifier(ls, a);

			for (size_t i = 0; i < num_keywords(); i++) {
				if (!strcmp(ls->tok->value, keywords[i].body)) {
					ls->tok->type = TOK_KEYWORD;
					ls->tok->keyword = &keywords[i];
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
			a = parse_interpolated_string(ls, a);
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
	ls->loc.len = 0;
	lexer_push_token(ls, TOK_END, a, a);
	token_rewind(&ls->tok);

	if (ls->r->fatal) {
		error_write(ls->r, stderr);
		token_clear(ls->tok);
		free_lexer(ls);
		return false;
	} else if (ls->r->pending) {
		error_write(ls->r, stderr);
	}

	m->tok = ls->tok;
	m->stage = MODULE_STAGE_LEXED;
	free_lexer(ls);

	return true;
}
