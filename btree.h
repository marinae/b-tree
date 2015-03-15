#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct DBC {
    size_t db_size;
    size_t page_size;
    size_t cache_size;
} DBC;

typedef struct BTree {
    int file;
    DBC *conf;
    size_t num_blocks;
} BTree;


BTree *btree_create(char *file, DBC *conf);
void btree_delete(BTree *bt);

