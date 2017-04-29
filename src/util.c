#include "util.h"

#include <string.h>

size_t line_number(const char* str, size_t location)
{
	size_t line_num = 1;
	for (size_t i = 0; i < strlen(str); i++) {
		if (str[i] == '\n') line_num++;
		if (i >= location) break;
	}
	return line_num;
}

size_t column_number(const char* str, size_t location)
{
	size_t column_num = 1;
	for (size_t i = 0; i < strlen(str); i++) {
		if (str[i] == '\n') column_num = 1;
		else column_num++;
		if (i >= location) break;
	}
	return column_num;
}

char* get_line(const char* str, size_t location)
{
	char* line = NULL;
	size_t line_end = 0;
	for (size_t i = location; i < strlen(str); i++)
		if (str[i] == '\n' || str[i] == 0) {
			line_end = i;
			break;
		}
	line = malloc(line_end - location + 1);
	strncpy(line, str + location, line_end - location);
	line[line_end - location] = 0;
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
