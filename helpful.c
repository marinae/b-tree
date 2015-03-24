#include "helpful.h"

//+----------------------------------------------------------------------------+
//| Compute free space in block                                                |
//+----------------------------------------------------------------------------+

size_t free_space(block *b, size_t block_size) {
    /* Check params */
    assert(b);
    assert(b->items || b->num_keys == 0);
    assert(b->children || b->num_children == 0);

    /* Busy space: */
    /* 1. sizeof(num_keys + num_children) */
    size_t busy = sizeof(size_t) * 2;

    for (size_t i = 0; i < b->num_keys; ++i) {
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

//+----------------------------------------------------------------------------+
//| Compare two keys if (key1 < key2)                                          |
//+----------------------------------------------------------------------------+

bool compare_keys(DBT *key1, DBT *key2) {
    /* Check params */
    assert(key1 && key1->data && key2 && key2->data);

    /* Assign shorter names */
    char *k1 = key1->data;
    char *k2 = key2->data;
    /* Compute minimum length */
    size_t n = key1->size;
    if (key2->size < n)
        n = key2->size;
    /* Return true if key1 < key2 */
    int result = strncmp(k1, k2, n);

    return (result < 0);
}

//+----------------------------------------------------------------------------+
//| Allocate memory and create item                                            |
//+----------------------------------------------------------------------------+

item *create_item(DBT *key, DBT *value) {
    /* Check params */
    assert(key && key->data && value && value->data);

    /* Create new item with specified key and value */
    item *it  = (item *)calloc(1, sizeof(item));
    it->key   = (DBT *)calloc(1, sizeof(DBT));
    it->value = (DBT *)calloc(1, sizeof(DBT));

    it->key->data   = (void *)calloc(1, key->size);
    it->value->data = (void *)calloc(1, value->size);

    it->key->size   = key->size;
    it->value->size = value->size;

    memcpy(it->key->data,   key->data,   key->size);
    memcpy(it->value->data, value->data, value->size);

    return it;
}

//+----------------------------------------------------------------------------+
//| Free block                                                                 |
//+----------------------------------------------------------------------------+

int free_block(block *b) {
    /* Check params */
    assert(b);

    /* Free all internal structures of b and block itself */
    if (b->items) {
        for (size_t i = 0; i < b->num_keys; ++i) {
            if (b->items[i])
                free(b->items[i]);
        }
        free(b->items);
    }
    if (b->children)
        free(b->children);
    free(b);

    return 0;
}

//+----------------------------------------------------------------------------+
//| Free item                                                                  |
//+----------------------------------------------------------------------------+

int free_item(item *it) {
    /* Check params */
    assert(it);

    if (it->key) {
        if (it->key->data)
            free(it->key->data);
        free(it->key);
    }
    if (it->value) {
        if (it->value->data)
            free(it->value->data);
        free(it->value);
    }
    return 0;
}

//+----------------------------------------------------------------------------+
//| Is there enough memory for key-value?                                      |
//+----------------------------------------------------------------------------+

bool enough_mem(DB *db, block *b, DBT *key, DBT *value) {
    /* Check params */
    assert(db && db->info && key && value);

    /* Compute free and needed memory size */
    size_t free_mem = free_space(b, db->info->block_size);
    size_t needed_mem = sizeof(size_t) * 2 + key->size + value->size;
    if (b->num_children > 0)
        needed_mem += 8;

    /* Compare them */
    return (needed_mem <= free_mem);
}

//+----------------------------------------------------------------------------+
//| Make k the new root index                                                  |
//+----------------------------------------------------------------------------+

int make_root(DB *db, size_t k) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);

    /* Make k the new root index with one child (old root) */
    free_block(db->root);
    db->root = (block *)calloc(1, sizeof(block));

    db->root->num_children = 1;
    db->root->children = (size_t *)calloc(1, sizeof(size_t));
    db->root->children[0] = db->info->root_index;
    db->info->root_index = k;

    return 0;
}