#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>

#include "prefix_tree.h"

#define BROADCAST_PORT 9876
#define BUFFER_SIZE 1024

struct __attribute__((packed)) ptree_word {
    char word[MAX_WORD_LENGTH];
    int col_words;
};

void async_handle_broadcast();
void send_ptree(prefix_tree *tree, int client_socket);

#endif //NET_UTILS_H
