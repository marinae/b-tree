#include "database.h"

// TODO: close database, delete key, insert key, select key

//+----------------------------------------------------------------------------+
//| Close database                                                             |
//+----------------------------------------------------------------------------+

int f_close(struct DB *db) {
    /* Check params */
    assert(db && db->info);

    return 0;
}

//+----------------------------------------------------------------------------+
//| Delete key                                                                 |
//+----------------------------------------------------------------------------+

int f_delete(struct DB *db, struct DBT *key) {
    /* Check params */
    assert(db && db->info && key && key->data);

    return 0;
}

//+----------------------------------------------------------------------------+
//| Insert key-value                                                           |
//+----------------------------------------------------------------------------+

int f_insert(struct DB *db, struct DBT *key, struct DBT *value) {
    /* Check params */
    assert(db && db->info && key && key->data && value && value->data);

    /* Find place for key */
    /* TODO: find RIGHT place for key in B-tree */
    size_t k = db->_find_empty_block(db);

    /* Check if there is no free block */
    if (k == 0)
        return -1;

    item **items = (item **)calloc(1, sizeof(item *));
    item *one = (item *)calloc(1, sizeof(item));
    one->key = key;
    one->value = value;
    items[0] = one;

    /* Fill new block */
    struct block b = {
        .num_keys = 1,
        .items = items,
        .num_children = 0,
        .children = NULL
    };

    /* Write block */
    return db->_write_block(db, k, &b);
}

//+----------------------------------------------------------------------------+
//| Select key-value                                                           |
//+----------------------------------------------------------------------------+

int f_select(struct DB *db, struct DBT *key, struct DBT *value) {
    /* Check params */
    assert(db && db->info && key && key->data && value);

    /* Find specific key */
    /* TODO: Returns sequence values. Do it RIGHT */
    struct block *b = db->_read_block(db, 170);
    if (!b)
        return -1;

    write(1, b->items[0]->key->data, b->items[0]->key->size);
    printf("\n");
    write(1, b->items[0]->value->data, b->items[0]->value->size);
    printf("\n");

    /* Free all allocated memory */
    for (int j = 0; j < b->num_keys; ++j) {
        free(b->items[j]->key->data);
        free(b->items[j]->value->data);
        free(b->items[j]->key);
        free(b->items[j]->value);
        free(b->items[j]);
    }
    free(b->items);
    free(b->children);
    free(b);

    return 0;
}

//+----------------------------------------------------------------------------+
//| Synchronize                                                                |
//+----------------------------------------------------------------------------+

int f_sync(struct DB *db) {
    /* Check params */
    assert(db && db->info);

    return 0;
}