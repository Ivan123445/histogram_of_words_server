#include "prefix_tree.h"

#include "../file_utils/file_utils.h"

prefix_tree *prefix_tree_init() {
    prefix_tree *ptree = malloc(sizeof(prefix_tree));
    ptree->root = NULL;
    memset(ptree->children, 0, sizeof(prefix_tree*)*ALPHABET_SIZE);
    ptree->words_here = false;
    ptree->character = 0;
    return ptree;
}

void prefix_tree_insert_word_with_col_words(prefix_tree *parent, const char *word, const size_t words_here) {
    for (size_t i = 0; i < 100000; i++) {}
    prefix_tree *cur_node = parent;
    for (;*word != '\0' && *word != ' ' && *word != '.' && *word != ','; ++word) {
        unsigned char ch = *word;
        if (cur_node->children[ch] != 0) {
            cur_node = cur_node->children[ch];
        } else {
            prefix_tree *tmp = malloc(sizeof(prefix_tree));
            cur_node->children[ch] = tmp;
            tmp->character = *word;
            tmp->words_here = 0;
            memset(tmp->children, 0, sizeof(prefix_tree*)*ALPHABET_SIZE);
            cur_node = tmp;
        }
    }
    cur_node->words_here += words_here;
}

void prefix_tree_insert_word(prefix_tree *parent, const char *word) {
    prefix_tree_insert_word_with_col_words(parent, word, 1);
}

void prefix_tree_insert_tree_recursive(prefix_tree* parent, const prefix_tree *child, char *buffer, const size_t depth) {
    buffer[depth] = child->character;
    if (child->words_here) {
        buffer[depth + 1] = '\0';
        prefix_tree_insert_word_with_col_words(parent, buffer, child->words_here);
    }
    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
        if (child->children[i] != 0) {
            prefix_tree_insert_tree_recursive(parent, child->children[i], buffer, depth + 1);
        }
    }
}

void prefix_tree_insert_tree(prefix_tree *parent, prefix_tree *child) {
    char *buffer = malloc(ALPHABET_SIZE * sizeof(char));
    memset(buffer, '\0', MAX_WORD_LENGTH);

    prefix_tree_insert_tree_recursive(parent, child, buffer, -1);

    free(buffer);
}

void *get_prefix_tree_by_text(void *arg) {
    thread_args *args = arg;
    char *filename = args->filename;
    long start = args->start;
    long end = args->end;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Не удалось открыть файл");
        exit(EXIT_FAILURE);
    }
    fseek(file, start, SEEK_SET);

    prefix_tree *ptree = prefix_tree_init();
    char buffer[MAX_WORD_LENGTH] = {0};
    while (ftell(file) < end - 1 && get_word(file, buffer, MAX_WORD_LENGTH) == 1) {
        prefix_tree_insert_word(ptree, buffer);
    }
    return ptree;
}


void prefix_tree_print_recursive(const prefix_tree *tree, char *buffer, const size_t depth) {
    buffer[depth] = tree->character;
    if (tree->words_here) {
        buffer[depth + 1] = '\0';
        printf("%s", buffer);
        printf(": %lu\n", tree->words_here);
    }
    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
        if (tree->children[i] != 0) {
            prefix_tree_print_recursive(tree->children[i], buffer, depth + 1);
        }
    }
}

void prefix_tree_print(const prefix_tree *ptree) {
    char *buffer = malloc(ALPHABET_SIZE * sizeof(char));
    memset(buffer, '\0', MAX_WORD_LENGTH);

    prefix_tree_print_recursive(ptree, buffer, -1);

    free(buffer);
}

void prefix_tree_destroy(prefix_tree *ptree) {
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (ptree->children[i] != 0) {
            prefix_tree_destroy(ptree->children[i]);
        }
    }
    free(ptree);
}






