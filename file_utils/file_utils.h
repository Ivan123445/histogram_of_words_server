#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>


#define FILENAME_PREFIX "/tmp/file_for_histogram"


char *generate_filename();
void receive_file(int client_socket, const char *output_filename);
long* split_file(char* filename, size_t col_parts);
int get_word(FILE *file, char* buffer, size_t buffer_size);

#endif //FILE_UTILS_H
