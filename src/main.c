#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

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

int main(int argc, char** argv)
{
	char* text = load_file(argv[1]);

	if (text) {
		struct Token* tok = lexer_tokenize(text);
		if (lexer_error()) {
			printf("%s", lexer_error());
		} else {
			printf("%s", lexer_dump(tok));
		}

		free(text);
	}

	return EXIT_SUCCESS;
}
