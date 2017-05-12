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

static void parse_escape_sequences(struct LexState *ls, char *str)
{
	char *out = str;
	size_t i = 0;
	do {
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
				i--;
				
				hex_sequence[j] = 0;
				//printf("found hex escape sequence: %s\n", hex_sequence);
				a = (char)strtol(hex_sequence, NULL, 16);
			} break;
			default: {
				ls->loc.index += i;
				ls->loc.len = 2;
				lexer_push_error(ls, ERR_WARNING, "unrecognized escape sequence: '\\%c'", a);
				continue;
			}
			}
		}
		
		*out++ = a;
	} while (str[i++]);
}

static char *parse_identifier(struct LexState *ls, char *ch)
{
	char *end = ch;
	while (is_legal_in_identifier(*end) && *end) end++;

	ls->loc.len = end - ch;
	token_push(ls->loc, TOK_IDENTIFIER, ch, end, &ls->tok);

	return end;
}

static char *parse_number(struct LexState *ls, char *ch)
{
	char *end = ch;

	if (!strncmp(ch, "0x", 2)) {
		end += 2;
		while (is_hex_digit(*end)) end++;

		ls->loc.len = end - ch;
		token_push(ls->loc, TOK_INTEGER, ch, end, &ls->tok);
		ls->tok->iData = (int64_t)strtol(ls->tok->value, NULL, 16);
	} else {
		bool is_integer = true;
		bool is_oct = (*ch == '0'), found_exponent = false;
		
		while (true) {
			if (*end == 'e') {
				if (found_exponent || is_integer) {
					ls->loc.len = end - ch;
					lexer_push_error(ls, ERR_FATAL, "incorrect use of scientific notation in numeric literal");
					break;
				}

				found_exponent = true;
				end++;
				if (*end == '-' || *end == '+') end++;
				continue;
			}
			
			if ((is_oct && is_oct_digit(*end)) || is_dec_digit(*end)) {
				end++;
				continue;
			}

			if (*end == '.') {
				if (is_integer) {
					is_integer = false;
				} else {
					/* we've run into a second . for this number, e.g. 0.1.2
					 * we will break, so that example will be parsed as 0.1, 0.2 */
					break;
				}
				
				end++;
				continue;
			}

			break;
		}

		ls->loc.len = end - ch;
		if (is_integer) {
			token_push(ls->loc, TOK_INTEGER, ch, end, &ls->tok);
			ls->tok->iData = (int64_t)strtol(ls->tok->value, NULL, 0);
		} else {
			token_push(ls->loc, TOK_FLOAT, ch, end, &ls->tok);
			ls->tok->fData = atof(ls->tok->value);
		}
	}

	return end;
}

static char *parse_string_literal(struct LexState *ls, char *ch)
{
	ch += 1;
	char *end = ch;
	do {
		if (*end == '\\') end++;
		end++;
	} while (*end != '\"' && *end != '\n' && *end);

	if (*end == 0 || *end == '\n') {
		SKIP_TO_END_OF_LINE(ch, end);

		ls->loc.len = end - ch + 1;
		lexer_push_error(ls, ERR_FATAL, "unterminated string literal");
		
		return end;
	}

	ls->loc.len = end - ch;
	token_push(ls->loc, TOK_STRING, ch, end, &ls->tok);
	parse_escape_sequences(ls, ls->tok->value);

	return end + 1;
}

static char *parse_character_literal(struct LexState *ls, char *ch)
{
	ch += 1; /* skip the ' */
	char *end = ch;
	do {
		if (*end == '\\') end++;
		if (*end) end++; /* we might skip over the end of the string without the conditional */
	} while (*end != '\'' && *end != '\n' && *end);

	if (*end == 0 || *end == '\n') {
		SKIP_TO_END_OF_LINE(ch, end);

		ls->loc.len = end - ch + 1;
		lexer_push_error(ls, ERR_FATAL, "unterminated character literal");
		
		return end;
	}

	ls->loc.len = end - ch;
	token_push(ls->loc, TOK_STRING, ch, end, &ls->tok);
	parse_escape_sequences(ls, ls->tok->value);

	if (strlen(ls->tok->value) != 1) {
		/* add 2 to the length because the body of the token doesn't include the ' characters */
		ls->loc.len = end - ch + 2;

		lexer_push_error(ls, ERR_WARNING, "multi-element character literal will be truncated to the first element");
	}

	ls->tok->type = TOK_INTEGER;
	ls->tok->iData = (int64_t)ls->tok->value[0];

	return end + 1;
}

static char *parse_raw_string_literal(struct LexState *ls, char *ch)
{
	ch += 2; /* skip the R( */
	char *end = ch;

	while (*end != ')' && *end) end++; /* find the end of the delimiter spec */

	if (*end == 0) {
		SKIP_TO_END_OF_LINE(ch, end)

		ls->loc.len = end - ch;
		lexer_push_error(ls, ERR_FATAL, "unterminated delimiter specification");

		return end;
	}

	char *delim = oak_malloc(end - ch + 1);
	strncpy(delim, ch, end - ch);
	delim[end - ch] = 0;
	size_t delim_len = strlen(delim);

	end++; /* skip the ) */
	ch = end;
	/* find the end of the string with the delimiter */
	while (strncmp(end, delim, delim_len) && *end) end++;

	if (*end == 0) {
		ls->loc.len = strlen(delim) + 3;
		lexer_push_error(ls, ERR_FATAL, "unterminated raw string literal");

		SKIP_TO_END_OF_LINE(ch, end)
		free(delim);

		return end;
	}

	ls->loc.len = strlen(delim) + 4 + (end - ch);
	token_push(ls->loc, TOK_STRING, ch, end, &ls->tok);
	free(delim);
	
	return end + delim_len;
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

static char *parse_operator(struct LexState *ls, char *ch)
{
	enum OpType op_type = match_operator(ch);
	size_t len = get_op_len(op_type);

	ls->loc.len = len;
	token_push(ls->loc, TOK_OPERATOR, ch, ch + len, &ls->tok);
	ls->tok->op_type = op_type;

	return ch + len;
}

struct Token *tokenize(struct LexState *ls)
{
	char *ch = ls->text;
	
	while (*ch) {
		ls->loc = (struct Location){ls->text, ls->file, ch - ls->text, -1};

		if (is_whitespace(*ch)) {
			while (is_whitespace(*ch) && *ch) {
				ch++;
			}
			continue;
		}

		if (!strncmp(ch, "/*", 2)) {
			ch += 2;
			char *end = ch;

			while (strncmp(end, "*/", 2) && *end) end++;

			if (*end == 0) {
				ls->loc.len = 2;
				error_push(ls->es, ls->loc, ERR_FATAL, "unmatched comment initializer");

				while (*ch != '\n' && *ch) ch++;
			} else {
				ch = end + 2;
			}
			
			continue;
		}

		if (!strncmp(ch, "*/", 2)) {
			ls->loc.len = 2;
			error_push(ls->es, ls->loc, ERR_FATAL, "unmatched comment terminator");
			ch += 2;

			continue;
		}

		if (!strncmp(ch, "//", 2)) {
			while (*ch != '\n' && *ch) ch++;

			continue;
		}

		if (!strncmp(ch, "R(", 2)) {
			ch = parse_raw_string_literal(ls, ch);
		} else if (is_identifier_start(*ch)) {
			ch = parse_identifier(ls, ch);
			
			if (keyword_get_type(ls->tok->value) != KEYWORD_INVALID) {
				ls->tok->type = TOK_KEYWORD;
				ls->tok->keyword_type = keyword_get_type(ls->tok->value);
			}

			if (!strcmp(ls->tok->value, "true") || !strcmp(ls->tok->value, "false")) {
				ls->tok->type = TOK_BOOL;
				ls->tok->bool_type = strcmp(ls->tok->value, "true") ? false : true;
			}
		} else if (*ch == '\"') {
			ch = parse_string_literal(ls, ch);
		} else if (is_dec_digit(*ch) || *ch == '.') {
			ch = parse_number(ls, ch);
		} else if (match_operator(ch) != OP_INVALID) {
			ch = parse_operator(ls, ch);
		} else if (*ch == '\'') {
			ch = parse_character_literal(ls, ch);
		} else {
			ls->loc.len = 1;
			token_push(ls->loc, (enum TokType)*ch, ch, ch + 1, &ls->tok);
			ch++;
		}
	}

	cat_strings(ls->tok);
	token_rewind(&ls->tok);

	return ls->tok;
}
