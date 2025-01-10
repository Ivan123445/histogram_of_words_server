#ifndef PREFIX_TREE_H
#define PREFIX_TREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../file_utils/file_utils.h"

#define MAX_WORD_LENGTH 20
#define ALPHABET_SIZE 75
#define ALPHABET_OFFSET 48

typedef struct prefix_tree {
    struct prefix_tree **childrens;
    char character;
    unsigned short col_children;
    unsigned short words_here;
} prefix_tree;

typedef struct {
    char *filename;
    long start;
    long end;
} thread_args;


prefix_tree *prefix_tree_init();
void *get_prefix_tree_by_text(void *arg);
void prefix_tree_insert_tree(prefix_tree *parent, prefix_tree *child);
void prefix_tree_print(const prefix_tree *ptree);
void prefix_tree_destroy(prefix_tree *ptree);


#endif // PREFIX_TREE_H
