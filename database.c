#include "database.h"

// TODO: f_close, f_delete, f_select

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