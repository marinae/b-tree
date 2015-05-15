#include "database.h"

//+----------------------------------------------------------------------------+
//| Close database                                                             |
//+----------------------------------------------------------------------------+

int f_close(DB *db) {
    /* Check params */
    assert(db && db->info && db->root);
    /* Sync cache to disc */
    flush_cache(db);
    /* Free DB parameters */
    free(db->info->bitmap);
    free(db->info);
    free_block(db->root);
    free(db);
    return 0;
}

//+----------------------------------------------------------------------------+
//| Synchronize                                                                |
//+----------------------------------------------------------------------------+

int f_sync(DB *db) {
    /* Check params */
    assert(db && db->info && db->root);
    flush_cache(db);
    // TODO: ?
    return 0;
}