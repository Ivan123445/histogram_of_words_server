#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "file_utils/file_utils.h"
#include "net_utils/net_utils.h"
#include "prefix_tree/prefix_tree.h"

#define NUM_THREADS_BY_DEFAULT 4

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
  for (int i = 0; i < num_parts; i++) {
    prefix_tree_insert_tree(main_ptree, prefix_trees[i]);
    prefix_tree_destroy(prefix_trees[i]);
  }

  free(threads);
  free(thread_args_massive);
  free(prefix_trees);
  return main_ptree;
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

int get_col_cores() {
  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (num_cores < 1) {
    num_cores = NUM_THREADS_BY_DEFAULT;
    printf("The number of cores could not be determined, the default value is used\n");
  }
  printf("Number of threads used: %d\n", num_cores);
  return num_cores;
}

int main(const int argc, char *argv[]) {
  handle_arguments(argc, argv);
  char *filename = generate_filename();
  const int num_threads = get_col_cores() - 1;

  async_handle_find_servers();
  async_handle_broadcast();
  int server_socket = get_server_socket();
  printf("Server started. Waiting for a connection...\n");
  while (true) {
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) == -1) {
      perror("Accept failed");
      close(server_socket);
      exit(EXIT_FAILURE);
    }

    printf("Client connected.\nReceiving file...\n");
    receive_file(client_socket, filename);
    printf("File received.\n");
    long* file_parts = split_file(filename, num_threads);
    printf("File splitted\n");
    prefix_tree *main_ptree = handle_file_parts_parallel(filename, file_parts, num_threads);
    // prefix_tree_print(main_ptree);
    printf("Sending ptree...\n");
    send_ptree(main_ptree, client_socket);
    printf("Sending completed\n");
    close(client_socket);
    prefix_tree_destroy(main_ptree);
    printf("Client disconnected.\n\n");
  }

  close(server_socket);
  return 0;
}