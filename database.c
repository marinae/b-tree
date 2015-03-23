#include "database.h"

// TODO: close database, delete key, insert key, select key, insert_nonfull

//+----------------------------------------------------------------------------+
//| Close database                                                             |
//+----------------------------------------------------------------------------+

int f_close(DB *db) {
    /* Check params */
    assert(db && db->info && db->root);

    return 0;
}

//+----------------------------------------------------------------------------+
//| Delete key                                                                 |
//+----------------------------------------------------------------------------+

int f_delete(DB *db, DBT *key) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(key && key->data);

    return 0;
}

//+----------------------------------------------------------------------------+
//| Insert key-value                                                           |
//+----------------------------------------------------------------------------+

int f_insert(DB *db, DBT *key, DBT *value) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(key && key->data && value && value->data);

    /* Find place for key */
    block *current = db->root;

    /* Compute free memory in block and needed memory */
    size_t free_mem = free_space(current, db->info->block_size);
    size_t needed_mem = sizeof(size_t) * 2 + key->size + value->size;
    if (current->num_children > 0)
        needed_mem += 8;

    /* Compare them */
    if (needed_mem <= free_mem)
        return insert_nonfull(db, current, db->info->root_index, key, value);

    /* Root is full -> split root */
    // s = allocate node
    // root = s
    // s.child = current
    // split child(s)
    // return insert_nonfull(s, key, value)

    return 0;
}

//+----------------------------------------------------------------------------+
//| Select key-value                                                           |
//+----------------------------------------------------------------------------+

int f_select(DB *db, DBT *key, DBT *value) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(key && key->data && value);

    /* Find specific key */
    /* TODO: Returns sequence values. Do it RIGHT */
    /*block *b = db->_read_block(db, 170);
    if (!b)
        return -1;

    write(1, b->items[0]->key->data, b->items[0]->key->size);
    printf("\n");
    write(1, b->items[0]->value->data, b->items[0]->value->size);
    printf("\n");*/

    /* Free all allocated memory */
    /*for (int j = 0; j < b->num_keys; ++j) {
        free(b->items[j]->key->data);
        free(b->items[j]->value->data);
        free(b->items[j]->key);
        free(b->items[j]->value);
        free(b->items[j]);
    }
    free(b->items);
    free(b->children);
    free(b);*/

    return 0;
}

//+----------------------------------------------------------------------------+
//| Synchronize                                                                |
//+----------------------------------------------------------------------------+

int f_sync(DB *db) {
    /* Check params */
    assert(db && db->info && db->root);

    return 0;
}

//+----------------------------------------------------------------------------+
//| Helpful functions                                                          |
//+----------------------------------------------------------------------------+

int insert_nonfull(DB *db, block *b, size_t k, DBT *key, DBT *value) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(key && key->data && value && value->data);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);

    /* Insert key into this block or one of its children */
    if (b->num_children == 0) {
        /* Leaf block */
    } else {
        /* Non-leaf block */
    }

    /*write(1, key->data, key->size);
    printf("\n");
    write(1, value->data, value->size);
    printf("\n");*/

    return 0;
}

size_t free_space(block *b, size_t block_size) {
    /* Check params */
    assert(b);
    assert(b->items || b->num_keys == 0);
    assert(b->children || b->num_children == 0);

    /* Busy space: */
    /* 1. sizeof(num_keys + num_children) */
    size_t busy = sizeof(size_t) * 2;

    for (int i = 0; i < b->num_keys; ++i) {
        /* 2. sizeof(key->size + value-> size) */
        busy += sizeof(size_t) * 2;

        /* 3. key->size */
        busy += b->items[i]->key->size;

        /* 4. value->size */
        busy += b->items[i]->value->size;
    }
    /* 5. num_children * sizeof(child) */
    busy += b->num_children * sizeof(size_t);

    return block_size - busy;
}