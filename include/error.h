#ifndef ERROR_H
#define ERROR_H

void push_error(char* str, size_t location, char* message);
void write_errors(FILE* fp);

#endif
