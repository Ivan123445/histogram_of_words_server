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
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>


#include "prefix_tree.h"

#define PORT 12346
#define BROADCAST_PORT 9876
#define BUFFER_SIZE 1024
#define MAX_PCS 10
#define NET_BUFFER_SIZE 1024

#define FIND_SERVERS_DELAY 3

struct __attribute__((packed)) ptree_word {
    char word[MAX_WORD_LENGTH];
    unsigned short col_words;
};

int get_server_socket();

void async_handle_broadcast();

void async_handle_find_servers();

void send_ptree(prefix_tree *tree, int client_socket);

#endif //NET_UTILS_H
