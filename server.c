#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "file_utils/file_utils.h"
#include "prefix_tree/prefix_tree.h"

#define NUM_THREADS 4
#define FILENAME "/tmp/file_for_histogram"
#define PORT 12345

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
  }

  free(threads);
  free(thread_args_massive);
  free(prefix_trees);
  return main_ptree;
}

int main(const int argc, char *argv[]) {
  char *filename = generate_filename();

  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

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

  printf("Waiting for a connection...\n");

  // Принимаем подключение от клиента
  if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) == -1) {
    perror("Accept failed");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  printf("Client connected. Sending file...\n");


  // Принимаем файл
  receive_file(client_socket, filename);
  long* file_parts = split_file(filename, NUM_THREADS);
  prefix_tree *main_ptree = handle_file_parts_parallel(filename, file_parts, NUM_THREADS);
  printf("Sending file...\n");

  close(server_socket);
  return 0;
}