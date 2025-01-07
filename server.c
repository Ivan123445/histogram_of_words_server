#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "file_utils/file_utils.h"
#include "net_utils/net_utils.h"
#include "prefix_tree/prefix_tree.h"

#define NUM_THREADS 4
#define FILENAME "/tmp/file_for_histogram"
#define PORT 12346

char *generate_filename() {
  srand(time(0));
  static char filename[100];
  int random_number = rand() % 10000;

  snprintf(filename, sizeof(filename), "%s_%d", FILENAME, random_number);
  return filename;
}

prefix_tree *handle_file_parts_parallel(char *filename, long *file_parts, size_t num_parts) {
  pthread_t *threads = malloc(sizeof(pthread_t) * num_parts);
  prefix_tree **prefix_trees = malloc(sizeof(prefix_tree*) * num_parts);
  thread_args *thread_args_massive = malloc(sizeof(thread_args) * num_parts);

  for (int i = 0; i < num_parts; i++) {
    thread_args_massive[i].filename = filename;
    thread_args_massive[i].start = file_parts[i];
    thread_args_massive[i].end = file_parts[i+1];

    if (pthread_create(&threads[i], NULL, get_prefix_tree_by_text, &thread_args_massive[i]) != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < num_parts; i++) {
    if (pthread_join(threads[i], (void**)&prefix_trees[i]) != 0) {
      perror("pthread_join");
      exit(EXIT_FAILURE);
    }
  }

  prefix_tree *main_ptree = prefix_tree_init();
  for (int i = 0; i < NUM_THREADS; i++) {
    prefix_tree_insert_tree(main_ptree, prefix_trees[i]);
    prefix_tree_destroy(prefix_trees[i]);
  }

  free(threads);
  free(thread_args_massive);
  free(prefix_trees);
  return main_ptree;
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

void handle_arguments(const int argc, char *argv[]) {
  if (argc < 2) {
    return;
  }
  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    printf("This is the server part of a program for creating a histogram of words. "
       "It needs to be run on the pc where the calculations will take place. "
       "The program can be run on the same pc as the client part.\n");
    exit(0);
  }

  printf("Wrong number of arguments\n");
  printf("For more info use --help\n");
  exit(EXIT_FAILURE);
}

int main(const int argc, char *argv[]) {
  handle_arguments(argc, argv);
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  int server_socket = get_server_socket();
  async_handle_broadcast();
  char *filename = generate_filename();
  printf("Server started. Waiting for a connection...\n");

  while (true) {
    int client_socket;
    // Принимаем подключение от клиента
    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) == -1) {
      perror("Accept failed");
      close(server_socket);
      exit(EXIT_FAILURE);
    }

    printf("Client connected. Receiving file...\n");
    // Принимаем файл
    receive_file(client_socket, filename);
    printf("File received.\n");
    long* file_parts = split_file(filename, NUM_THREADS);
    prefix_tree *main_ptree = handle_file_parts_parallel(filename, file_parts, NUM_THREADS);
    prefix_tree_print(main_ptree);
    printf("Sending ptree...\n");
    send_ptree(main_ptree, client_socket);
    printf("Sending completed\n");
    prefix_tree_destroy(main_ptree);
    close(client_socket);
    printf("Client disconnected.\n");
  }

  close(server_socket);
  return 0;
}