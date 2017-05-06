#include "lexer.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_HEX_ESCAPE_SEQUENCE_LEN 64
#define MAX_OCT_ESCAPE_SEQUENCE_LEN 64

static void parse_escape_sequences(struct ErrorState* es, struct Location loc, char* str)
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
				loc.index += i; loc.len = 2;
				error_push(es, loc, ERR_WARNING, "unrecognized escape sequence: '\\%c'", a);
				continue;
			}
			}
		}
		
		*out++ = a;
	} while (str[i++]);
}

static char* parse_identifier(char* ch, struct Location loc, struct Token** tok)
{
	char* end = ch;
	while (is_legal_in_identifier(*end) && *end) end++;

	token_push(loc, TOK_IDENTIFIER, ch, end, tok);

	return end;
}

static char* parse_number(struct ErrorState* es, char* ch, struct Location loc, struct Token** tok)
{
	char* end = ch;

	if (!strncmp(ch, "0x", 2)) {
		end += 2;
		while (is_hex_digit(*end)) end++;

		token_push(loc, TOK_INTEGER, ch, end, tok);
		(*tok)->iData = (int64_t)strtol((*tok)->value, NULL, 16);
	} else {
		bool is_integer = true;
		bool is_oct = (*ch == '0'), found_exponent = false;
		
		while (true) {
			if (*end == 'e') {
				if (found_exponent || is_integer) {
					loc.len = end - ch;
					error_push(es, loc, ERR_FATAL, "incorrect use of scientific notation in numeric literal");
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
		
		if (is_integer) {
			token_push(loc, TOK_INTEGER, ch, end, tok);
			(*tok)->iData = (int64_t)strtol((*tok)->value, NULL, 0);
		} else {
			token_push(loc, TOK_FLOAT, ch, end, tok);
			(*tok)->fData = atof((*tok)->value);
		}
	}

	return end;
}

static char* parse_string_literal(struct ErrorState* es, struct Location loc, char* ch, struct Token** tok)
{
	ch += 1;
	char* end = ch;
	do {
		if (*end == '\\') end++;
		end++;
	} while (*end != '\"' && *end != '\n' && *end);

	if (*end == 0 || *end == '\n') {
		end = ch;
		while (*end != '\n' && *end) end++;
		loc.len = end - ch + 1;

		error_push(es, loc, ERR_FATAL, "unterminated string literal");
		
		return end;
	}

	token_push(loc, TOK_STRING, ch, end, tok);
	parse_escape_sequences(es, loc, (*tok)->value);

	return end + 1;
}

static char* parse_character_literal(struct ErrorState* es, struct Location loc, char* ch, struct Token** tok)
{
	ch += 1;
	char* end = ch;
	do {
		if (*end == '\\') end++;
		end++;
	} while (*end != '\'' && *end != '\n' && *end);

	if (*end == 0 || *end == '\n') {
		end = ch;
		while (*end != '\n' && *end) end++;
		loc.len = end - ch + 1;

		error_push(es, loc, ERR_FATAL, "unterminated character literal");
		
		return end;
	}

	token_push(loc, TOK_STRING, ch, end, tok);
	parse_escape_sequences(es, loc, (*tok)->value);

	if (strlen((*tok)->value) != 1) {
		/* add 2 to the length because the body of the token doesn't include the ' characters */
		loc.len = end - ch + 2;

		error_push(es, loc, ERR_WARNING, "multi-element character literal will be truncated to the first character");
	}

	(*tok)->type = TOK_INTEGER;
	(*tok)->iData = (int64_t)(*tok)->value[0];

	return end + 1;
}

static char* parse_raw_string_literal(struct ErrorState* es, char* ch, struct Location loc, struct Token** tok)
{
	ch += 2; /* skip the R( */
	char* end = ch;
	while (*end != ')' && *end) end++;
	if (*end == 0) {
		end = ch;
		while (*end != '\n' && *end) end++;
		loc.len = end - ch;
		error_push(es, loc, ERR_FATAL, "unterminated delimiter specification");
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
		loc.len = strlen(delim) + 3;
		error_push(es, loc, ERR_FATAL, "unterminated raw string literal");
		end = ch;
		while (*end != '\n' && *end) end++;
		free(delim);
		return end;
	}

	token_push(loc, TOK_STRING, ch, end, tok);
	free(delim);
	
	return end + delim_len;
}

static char* smart_cat(char* first, char* second)
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
static bool cat_strings(struct Token* tok)
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

static char* parse_operator(char* ch, struct Location loc, struct Token** tok)
{
	enum OpType op_type = match_operator(ch);
	size_t len = get_op_len(op_type);

	token_push(loc, TOK_OPERATOR, ch, ch + len, tok);
	(*tok)->op_type = op_type;

	return ch + len;
}

struct Token* tokenize(struct ErrorState* es, char* code, char* filename)
{
	struct Token *tok = NULL;
	char* ch = code;
	
	while (*ch) {
		struct Location loc = (struct Location){code, filename, ch - code, -1};

		if (is_whitespace(*ch)) {
			while (is_whitespace(*ch) && *ch) {
				ch++;
			}
			continue;
		}

		if (!strncmp(ch, "/*", 2)) {
			ch += 2;
			char* end = ch;

			while (strncmp(end, "*/", 2) && *end) end++;

			if (*end == 0) {
				loc.len = 2;
				error_push(es, loc, ERR_FATAL, "unmatched comment initializer");
				while (*ch != '\n' && *ch) ch++;
			}
			else ch = end + 2;
			continue;
		}

		if (!strncmp(ch, "*/", 2)) {
			loc.len = 2;
			error_push(es, loc, ERR_FATAL, "unmatched comment terminator");
			ch += 2;
			continue;
		}

		if (!strncmp(ch, "//", 2)) {
			while (*ch != '\n' && *ch) ch++;
			continue;
		}

		if (!strncmp(ch, "R(", 2)) {
			ch = parse_raw_string_literal(es, ch, loc, &tok);
		} else if (is_identifier_start(*ch)) {
			ch = parse_identifier(ch, loc, &tok);
		} else if (*ch == '\"') {
			ch = parse_string_literal(es, loc, ch, &tok);
		} else if (is_dec_digit(*ch) || *ch == '.') {
			ch = parse_number(es, ch, loc, &tok);
		} else if (match_operator(ch) != OP_INVALID) {
			ch = parse_operator(ch, loc, &tok);
		} else if (*ch == '\'') {
			ch = parse_character_literal(es, loc, ch, &tok);
		} else {
			token_push(loc, TOK_SYMBOL, ch, ch + 1, &tok);
			ch++;
		}
	}

	cat_strings(tok);
	token_rewind(&tok);

	return tok;
}
