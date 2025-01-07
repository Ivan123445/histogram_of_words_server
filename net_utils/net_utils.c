#include "net_utils.h"


void send_ptree_recursive(const prefix_tree *tree, char *buffer, const size_t depth, int client_socket) {
    buffer[depth] = tree->character;
    if (tree->words_here) {
        buffer[depth + 1] = '\0';
        struct ptree_word pword;
        memcpy(&pword.word, buffer, strlen(buffer)+1);
        pword.col_words = htonl(tree->words_here);

        send(client_socket, (char *)&pword, sizeof(pword), 0);
    }
    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
        if (tree->children[i] != 0) {
            send_ptree_recursive(tree->children[i], buffer, depth + 1, client_socket);
        }
    }
}

void send_ptree(prefix_tree *tree, int client_socket) {
    char *buffer = malloc(ALPHABET_SIZE * sizeof(char));
    memset(buffer, '\0', MAX_WORD_LENGTH);

    send_ptree_recursive(tree, buffer, -1, client_socket);

    free(buffer);
}


void *handle_broadcast() {
    int sock;
    struct sockaddr_in server_addr, client_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(BROADCAST_PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening for broadcast messages...\n");

    while (1) {
        char buffer[BUFFER_SIZE];
        socklen_t addr_len = sizeof(client_addr);
        ssize_t received = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &addr_len);

        if (received < 0) {
            perror("Error receiving broadcast message");
            continue;
        }

        buffer[received] = '\0';
        printf("Received broadcast message: %s from %s\n", buffer, inet_ntoa(client_addr.sin_addr));

        sendto(sock, NULL, 0, 0, (struct sockaddr *)&client_addr, addr_len);
    }
    close(sock);
    return NULL;
}

void async_handle_broadcast() {
    pthread_t broadcast_thread;
    if (pthread_create(&broadcast_thread, NULL, handle_broadcast, NULL) != 0) {
        perror("handle_broadcast");
        exit(EXIT_FAILURE);
    }
}
















