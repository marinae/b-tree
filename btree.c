#include "btree.h"

BTree *btree_create(char *file, DBC *conf) {
    
    // initialize with NULL and try to allocate memory
    BTree *bt = NULL;
    bt = (BTree*)calloc(1, sizeof(BTree));
    
    // check if memory is allocated
    if (!bt) {
        return NULL;
    }
    
    // try to open file for database
    bt->file = open(file, O_RDWR, S_IRWXU);
    
    // check if file exists
    if (bt->file < 0) {
        bt->file = open(file, O_CREAT | O_RDWR, S_IRWXU);
        
        // check if file was created
        if (bt->file < 0) {
            btree_delete(bt);
            return NULL;        }
        
        // params to write
        size_t page = conf->page_size;
        bt->num_blocks = 0;
        ssize_t written;
        
        written = write(bt->file, (void*)&page, sizeof(page));
        printf("Written bytes: %zd, sizeof(page) = %lu\n", written, sizeof(page));
        
        written = write(bt->file, (void*)&page, sizeof(page));
        printf("Written bytes: %zd, sizeof(page) = %lu\n", written, sizeof(page));
    }
    
    /*lseek(bt->file, 0, SEEK_SET);
    size_t x;
    read(bt->file, (void*)&x, sizeof(x));
    printf("Read size: %lu\n", x);*/
    
    // fill database parameters
    bt->conf = conf;
    
    // return pointer to created database
    return bt;
}

void btree_delete(BTree *bt) {
    
    if (bt) {
        // close database file if it is opened
        if (bt->file >= 0)
            close(bt->file);
        
        // free memory if is allocated
        free(bt);
        bt = NULL;
    }
}

int main() {

    char *file = "db";
    DBC conf = {256*1024*1024, 512, 256*1024};

    BTree *bt = btree_create("db", &conf);
    
    if (bt) {
        
        /*printf("btree is not NULL\n");
        printf("Descriptor: %d\n", bt->file);
        printf("DB size: %lu\n", bt->conf->db_size);
        printf("Page size: %lu\n", bt->conf->page_size);
        printf("Cache size: %lu\n", bt->conf->cache_size);*/
        
        btree_delete(bt);
        
    } else {
        
        printf("btree is NULL\n");
    }
    
    return 0;
}