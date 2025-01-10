#include "prefix_tree.h"

#include "../file_utils/file_utils.h"

prefix_tree *prefix_tree_init() {
    prefix_tree *ptree = malloc(sizeof(prefix_tree));
    ptree->words_here = 0;
    ptree->character = 0;
    ptree->col_children = 0;
    ptree->childrens = NULL;
    return ptree;
}

prefix_tree *prefix_tree_find_in_children(prefix_tree *ptree, char ch) {
    for (int i = 0; i < ptree->col_children; ++i) {
        if (ptree->childrens[i]->character == ch) {
            return ptree->childrens[i];
        }
    }
    return NULL;
}

prefix_tree *prefix_tree_add_children(prefix_tree *ptree, char ch) {
    prefix_tree *tmp = prefix_tree_init();
    tmp->character = ch;

    prefix_tree **new_childrens = malloc(sizeof(prefix_tree*) * (ptree->col_children + 1));
    for (int i = 0; i < ptree->col_children; ++i) {
        new_childrens[i] = ptree->childrens[i];
    }
    new_childrens[ptree->col_children] = tmp;
    free(ptree->childrens);
    ptree->childrens = new_childrens;
    ptree->col_children++;
    return tmp;
}

void prefix_tree_insert_word_with_col_words(prefix_tree *parent, const char *word, const size_t words_here) {
    // for (size_t i = 0; i < 100000; i++) {}
    prefix_tree *cur_node = parent;
    for (;*word != '\0' && *word != ' ' && *word != '.' && *word != ','; ++word) {
        const char ch = *word;
        prefix_tree *tmp = prefix_tree_find_in_children(cur_node, ch);
        if (tmp != NULL) {
            cur_node = tmp;
        } else {
            cur_node = prefix_tree_add_children(cur_node, ch);
        }
    }
    cur_node->words_here += words_here;
}

void prefix_tree_insert_word(prefix_tree *parent, const char *word) {
    prefix_tree_insert_word_with_col_words(parent, word, 1);
}

void prefix_tree_insert_tree_recursive(prefix_tree* parent, const prefix_tree *child, char *buffer, int depth) {
    if (depth >= 0) {
        buffer[depth] = child->character;
    }
    if (child->words_here) {
        buffer[depth + 1] = '\0';
        prefix_tree_insert_word_with_col_words(parent, buffer, child->words_here);
    }
    for (size_t i = 0; i < child->col_children; i++) {
        prefix_tree_insert_tree_recursive(parent, child->childrens[i], buffer, depth + 1);
    }
}

void prefix_tree_insert_tree(prefix_tree *parent, prefix_tree *child) {
    char *buffer = malloc(MAX_WORD_LENGTH * sizeof(char)+1);
    memset(buffer, '\0', MAX_WORD_LENGTH * sizeof(char)+1);

    prefix_tree_insert_tree_recursive(parent, child, buffer, -1);

    free(buffer);
}

void *get_prefix_tree_by_text(void *arg) {
    thread_args *args = arg;
    char *filename = args->filename;
    long start = args->start;
    long end = args->end;

    printf("Filename: %s\n", filename);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Не удалось открыть файл");
        exit(EXIT_FAILURE);
    }
    fseek(file, start, SEEK_SET);

    printf("Start creating ptree\n");
    prefix_tree *ptree = prefix_tree_init();
    char buffer[MAX_WORD_LENGTH] = {0};
    while (ftell(file) < end - 1 && get_word(file, buffer, MAX_WORD_LENGTH) == 1) {
        prefix_tree_insert_word(ptree, buffer);
        // printf("Ftell: %ld\n", ftell(file));
    }
    printf("End creating ptree\n");
    return ptree;
}


void prefix_tree_print_recursive(const prefix_tree *tree, char *buffer, int depth) {
    if (depth >= 0) {
        buffer[depth] = tree->character;
    }
    if (tree->words_here) {
        buffer[depth + 1] = '\0';
        printf("%s", buffer);
        printf(": %d\n", tree->words_here);
    }
    for (size_t i = 0; i < tree->col_children; i++) {
        prefix_tree_print_recursive(tree->childrens[i], buffer, depth + 1);
    }
}

void prefix_tree_print(const prefix_tree *ptree) {
    char *buffer = malloc(MAX_WORD_LENGTH * sizeof(char)+1);
    memset(buffer, '\0', MAX_WORD_LENGTH * sizeof(char)+1);

    prefix_tree_print_recursive(ptree, buffer, -1);

    free(buffer);
}

void prefix_tree_destroy(prefix_tree *ptree) {
    for (int i = 0; i < ptree->col_children; i++) {
        prefix_tree_destroy(ptree->childrens[i]);
    }
    free(ptree->childrens);
    free(ptree);
}






