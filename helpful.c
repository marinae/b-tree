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
        /* 2. sizeof(key->size + value->size) */
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

int compare_keys(DBT *key1, DBT *key2) {
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
    return strncmp(k1, k2, n);
}

//+----------------------------------------------------------------------------+
//| Compare two keys if (key1 == key2)                                         |
//+----------------------------------------------------------------------------+

bool equal_keys(DBT *key1, DBT *key2) {
    /* Check params */
    assert(key1 && key1->data && key2 && key2->data);

    /* Assign shorter names */
    char *k1 = key1->data;
    char *k2 = key2->data;
    /* Compare size of keys */
    if (key1->size != key2->size)
        return 0;
    /* Return true if key1 == key2 */
    int result = strncmp(k1, k2, key1->size);

    return (result == 0);
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

    /* Compute needed memory size */
    size_t needed_mem = need_memory(b);
    needed_mem += sizeof(size_t) * 2 + key->size + value->size;
    if (b->num_children > 0)
        needed_mem += 8;

    /* Compare them */
    return (needed_mem <= db->info->block_size);
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

    return db->_mark_block(db, k, 1);
}

//+----------------------------------------------------------------------------+
//| Needed memory for block                                                    |
//+----------------------------------------------------------------------------+

size_t need_memory(block *x) {
    /* Check params */
    assert(x);

    /* Busy space: */
    /* 1. sizeof(num_keys + num_children) */
    size_t busy = sizeof(size_t) * 2;

    for (size_t i = 0; i < x->num_keys; ++i) {
        /* 2. sizeof(key->size + value->size) */
        busy += sizeof(size_t) * 2;
        /* 3. key->size */
        busy += x->items[i]->key->size;
        /* 4. value->size */
        busy += x->items[i]->value->size;
    }
    /* 5. num_children * sizeof(child) */
    busy += x->num_children * sizeof(size_t);
    return busy;
}

int print_tree(DB *db, block *cur) {
    if (cur->num_children > 0) {
        block *b = db->_read_block(db, cur->children[0]);
        print_tree(db, b);
        free_block(b);
        for (size_t i = 1; i < cur->num_children; ++i) {
            printf("******************** Separator:\n");
            printf("%s\n", cur->items[i - 1]->key->data);
            b = db->_read_block(db, cur->children[i]);
            print_tree(db, b);
            free_block(b);
        }
    } else {
        printf("********** Leaf:\n");
        for (size_t i = 1; i < cur->num_keys; ++i) {
            printf("%s\n", cur->items[i]->key->data);
        }
    }
    return 0;
}