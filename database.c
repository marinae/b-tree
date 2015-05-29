#include "database.h"

//+----------------------------------------------------------------------------+
//| Close database                                                             |
//+----------------------------------------------------------------------------+

int f_close(DB *db) {
    /* Sync cache to disc */
    if (db && db->cache)
        flush_cache(db);
    /* Close WAL */
    if (db && db->logger)
        log_close(db);
    /* Close DB file */
    if (db && db->info)
        close(db->info->fd);

    /* Free DB parameters */
    if (db && db->info && db->info->bitmap)
        free(db->info->bitmap);
    if (db && db->info && db->info->hdr)
        free(db->info->hdr);
    if (db && db->info)
        free(db->info);
    if (db->logger)
        free(db->logger);
    if (db->root)
        free_block(db->root);
    if (db)
        free(db);
    
    return 0;
}

//+----------------------------------------------------------------------------+
//| Synchronize                                                                |
//+----------------------------------------------------------------------------+

int f_sync(DB *db) {
    /* Check params */
    assert(db && db->info && db->root);
    // TODO: scan WAL and do recovery
    return 0;
}