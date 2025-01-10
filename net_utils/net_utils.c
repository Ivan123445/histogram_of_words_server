#include "net_utils.h"

char server_ips[MAX_PCS][INET_ADDRSTRLEN];
int server_count = 0;

ssize_t send_all(int sock, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const char *ptr = buffer;

    while (total_sent < length) {
        ssize_t sent = send(sock, ptr + total_sent, length - total_sent, 0);
        if (sent < 0) {
            perror("send");
            return -1;
        }
        total_sent += sent;
    }

    return total_sent;
}

void send_ptree_recursive(const prefix_tree *tree, char *buffer, int depth, int client_socket) {
    if (depth >= 0) {
        buffer[depth] = tree->character;
    }
    if (tree->words_here) {
        buffer[depth + 1] = '\0';
        struct ptree_word pword;
        memcpy(&pword.word, buffer, strlen(buffer));
        pword.col_words = htons(tree->words_here);

        if (send_all(client_socket, &pword, sizeof(struct ptree_word)) < 0) {
            perror("send_ptree: ");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < tree->col_children; i++) {
        send_ptree_recursive(tree->childrens[i], buffer, depth + 1, client_socket);
    }
}

void send_ptree(prefix_tree *tree, int client_socket) {
    char *buffer = malloc(MAX_WORD_LENGTH * sizeof(char));
    memset(buffer, '\0', MAX_WORD_LENGTH * sizeof(char));

    send_ptree_recursive(tree, buffer, -1, client_socket);

    free(buffer);
}

int get_server_socket() {
    int server_socket;
    struct sockaddr_in server_addr;

    // Создаем сокет
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Настроим серверный адрес
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Все локальные IP-адреса
    server_addr.sin_port = htons(PORT);

    // Привязываем сокет к адресу
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Ожидаем подключений
    if (listen(server_socket, 1) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    return server_socket;
}

void find_servers(char server_ips[][INET_ADDRSTRLEN], int *server_count) {
    int sock;
    struct sockaddr_in broadcast_addr;
    char buffer[NET_BUFFER_SIZE];
    fd_set read_fds;
    struct timeval timeout;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    broadcast_addr.sin_port = htons(BROADCAST_PORT);

    if (sendto(sock, NULL, 0, 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("Broadcast message send failed");
        close(sock);
        exit(EXIT_FAILURE);
    }


    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);

    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    *server_count = 0;
    while (select(sock + 1, &read_fds, NULL, NULL, &timeout) > 0) {
        struct sockaddr_in server_addr;
        socklen_t addr_len = sizeof(server_addr);
        ssize_t received = recvfrom(sock, NULL, 0, 0, (struct sockaddr *)&server_addr, &addr_len);

        if (received < 0) {
            perror("Error receiving response");
            continue;
        }

        strncpy(server_ips[*server_count], inet_ntoa(server_addr.sin_addr), INET_ADDRSTRLEN);
        (*server_count)++;
    }

    close(sock);
}

void *find_servers_in_cycle() {
    while (true) {
        find_servers(server_ips, &server_count);
        // printf("Server count: %d\n", server_count);
        // printf("Server ips: ");
        // for (int i = 0; i < server_count; i++) {
        //     printf("%s, ", server_ips[i]);
        // }
        // printf("\n");
        sleep(FIND_SERVERS_DELAY);
    }
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
        socklen_t addr_len = sizeof(client_addr);
        ssize_t received = recvfrom(sock, NULL, 0, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (received < 0) {
            perror("Error receiving broadcast message");
            continue;
        }

        char ips_buffer[MAX_PCS * INET_ADDRSTRLEN + 1];
        memset(ips_buffer, 0, sizeof(ips_buffer));
        // Копирование IP адресов в буфер
        for (int i = 0; i < server_count; i++) {
            strcat(ips_buffer, server_ips[i]);
            strcat(ips_buffer, "|");
        }
        strcat(ips_buffer, "||");
        sendto(sock, ips_buffer, strlen(ips_buffer), 0, (struct sockaddr *)&client_addr, addr_len);
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

void async_handle_find_servers() {
    pthread_t find_server_thread;
    if (pthread_create(&find_server_thread, NULL, find_servers_in_cycle, NULL) != 0) {
        perror("find_servers_in_cycle");
        exit(EXIT_FAILURE);
    }
}















