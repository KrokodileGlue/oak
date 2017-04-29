#include "util.h"

#include <string.h>

size_t line_number(struct Location loc)
{
	size_t line_num = 1;
	for (size_t i = 0; i < strlen(loc.text); i++) {
		if (loc.text[i] == '\n') line_num++;
		if (i >= loc.index) break;
	}
	return line_num;
}

size_t column_number(struct Location loc)
{
	size_t column_num = 1;
	for (size_t i = 0; i < strlen(loc.text); i++) {
		if (loc.text[i] == '\n') column_num = 1;
		else column_num++;
		if (i >= loc.index) break;
	}
	return column_num;
}

size_t index_in_line(struct Location loc)
{
	size_t start = loc.index, end = loc.index;
	while (start) { /* find the beginning of the line we're in */
		if (loc.text[start] == '\n') {
			start++;
			break;
		}
		start--;
	}
	return loc.index - start;
}

char* get_line(struct Location loc)
{
	size_t start = loc.index, end = loc.index;
	while (start) { /* find the beginning of the line we're in */
		if (loc.text[start] == '\n') {
			start++;
			break;
		}
		start--;
	}
	while (is_whitespace(loc.text[start])) { start++; } /* skip any leading whitespace */
	while (loc.text[end] != '\n' && loc.text[end]) { end++; } /* find the end */

	char* line = malloc(end - start + 1);
	strncpy(line, loc.text + start, end - start);
	line[end - start] = 0;
	return line;
}

void* oak_malloc(size_t size)
{
	void* ptr = malloc(size);
	if (!ptr) {
		printf("out of memory\n");
		exit(EXIT_FAILURE);
	}
	return ptr;
}

char* load_file(const char* path)
{
	char* buf = NULL;
	FILE* file = fopen(path, "r");
	
	if (!file) return NULL;

	if (fseek(file, 0L, SEEK_END) == 0) {
		long len = ftell(file);
		if (len == -1) return NULL;
		
		buf = oak_malloc(len + 1);

		if (fseek(file, 0L, SEEK_SET) != 0)
			return NULL;

		size_t new_len = fread(buf, 1, len, file);
		if (ferror(file) != 0) {
			printf("could not read file %s\n", path);
			exit(EXIT_FAILURE);
		} else {
			buf[new_len++] = '\0';
		}
	}
	
	fclose(file);
	
	return buf;
}
