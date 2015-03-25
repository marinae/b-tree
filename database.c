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
//| Synchronize                                                                |
//+----------------------------------------------------------------------------+

int f_sync(DB *db) {
    /* Check params */
    assert(db && db->info && db->root);

    return 0;
}