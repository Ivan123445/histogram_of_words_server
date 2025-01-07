#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stddef.h>
#include <stdio.h>


void receive_file(int client_socket, const char *output_filename);
long* split_file(char* filename, size_t col_parts);
int get_word(FILE *file, char* buffer, size_t buffer_size);

#endif //FILE_UTILS_H
