#include "util.h"

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
