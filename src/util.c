#include "util.h"

#include <string.h>

size_t
line_len(struct location loc)
{
	size_t i;
	for (i = loc.index; loc.text[i] && loc.text[i] != '\n'; i++);
	return i - loc.index;
}

size_t
line_number(struct location loc)
{
	size_t line_num = 1;
	for (size_t i = 0; i < strlen(loc.text); i++) {
		if (loc.text[i] == '\n') line_num++;
		if (i >= loc.index) break;
	}
	return line_num;
}

size_t
column_number(struct location loc)
{
	size_t column_num = 1;
	for (size_t i = 0; i < strlen(loc.text); i++) {
		if (loc.text[i] == '\n') column_num = 1;
		else column_num++;
		if (i >= loc.index) break;
	}
	return column_num;
}

size_t
index_in_line(struct location loc)
{
	size_t start = loc.index;
	while (start) { /* find the beginning of the line we're in */
		if (loc.text[start] == '\n') {
			start++;
			break;
		}
		start--;
	}
	return loc.index - start;
}

char *substr(const char *str, size_t x, size_t y)
{
	char *ret = oak_malloc(y - x + 1);

	strncpy(ret, str + x, y - x);
	ret[y - x] = 0;

	return ret;
}

char *
smart_cat(char *first, char *second)
{
	size_t len = strlen(first) + strlen(second);

	first = realloc(first, len + 1);
	strcpy(first + strlen(first), second);

	first[len] = 0;

	return first;
}

char *new_cat(char *first, char *second)
{
	size_t len = strlen(first) + strlen(second);
	char *ret = oak_malloc(len + 1);

	strcpy(ret, first);
	strcpy(ret + strlen(first), second);

	ret[len] = 0;

	return ret;
}

char *
get_line(struct location loc)
{
	size_t start = loc.index, end = loc.index;
	while (start) { /* find the beginning of the line we're in */
		if (loc.text[start] == '\n') {
			start++;
			break;
		}
		start--;
	}

	while (loc.text[end] != '\n' && loc.text[end]) { end++; } /* find the end */

	char *line = malloc(end - start + 1);
	strncpy(line, loc.text + start, end - start);
	line[end - start] = 0;
	return line;
}

char *
strclone(char *str)
{
	char *ret = oak_malloc(strlen(str) + 1);
	strcpy(ret, str);
	return ret;
}

void
chop_extension(char *str)
{
	char *a = str;
	while (*a != '.' && *a)
		a++;
	*a = 0;
}

void
print_escaped_string(FILE *f, char *str, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		switch (str[i]) {
		case '\n': fprintf(f, "\\n"); break;
		case '\t': fprintf(f, "\\t"); break;
		default: fputc(str[i], f); break;
		}
	}
}

char *
add_extension(char *str)
{
	size_t len = strlen(str) + 3;

	str = oak_realloc(str, len);
	strcat(str, ".k");

	return str;
}

uint64_t
hash(char *d, size_t len)
{
	uint64_t hash = 5381;

	for (size_t i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + d[i];

	return hash;
}

void *
oak_malloc(size_t size)
{
	void *ptr = malloc(size);

	if (!ptr) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	return ptr;
}

void *
oak_realloc(void *mem, size_t size)
{
	void *ptr = realloc(mem, size);

	if (!ptr) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	return ptr;
}

char *
load_file(const char *path)
{
	char *buf = NULL;
	FILE *file = fopen(path, "r");

	if (!file) {
		DOUT("could not load file %s", path);
		return NULL;
	}

	if (fseek(file, 0L, SEEK_END) == 0) {
		long len = ftell(file);
		if (len == -1) return NULL;

		buf = oak_malloc(len + 1);

		if (fseek(file, 0L, SEEK_SET) != 0)
			return NULL;

		size_t new_len = fread(buf, 1, len, file);
		if (ferror(file) != 0) {
			DOUT("could not read file %s", path);
			exit(EXIT_FAILURE);
		} else {
			buf[new_len++] = '\0';
		}
	}

	fclose(file);

	return buf;
}
