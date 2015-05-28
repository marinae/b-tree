#include "database.h"

//+----------------------------------------------------------------------------+
//| Close database                                                             |
//+----------------------------------------------------------------------------+

int f_close(DB *db) {
    /* Check params */
    assert(db && db->info && db->root);
    /* Sync cache to disc */
    flush_cache(db);
    /* Close WAL */
    log_close(db);
    /* Close DB file */
    close(db->info->fd);
    /* Free DB parameters */
    free(db->info->bitmap);
    free(db->info);
    free(db->logger);
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
    // TODO: scan WAL
    return 0;
}