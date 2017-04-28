#ifndef ERROR_H
#define ERROR_H

void push_error(char* str, char* message, size_t location);
void write_errors(FILE* fp);

#endif
