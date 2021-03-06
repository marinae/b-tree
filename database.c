#include "database.h"

//+----------------------------------------------------------------------------+
//| Close database                                                             |
//+----------------------------------------------------------------------------+

int f_close(DB *db) {
    /* Sync cache to disc */
    //if (db && db->cache && db->logger)
        //flush_cache(db, db->logger->log_count);
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

    /* Seek last checkpoint */
    log_seek(db);

    Record *rec;

    while ((rec = log_read_next(db))) {
        /* Check block LSN in database */
        size_t lsn = get_lsn(db, rec->block_id);

        if (lsn < rec->b->lsn) {
            #ifdef _DEBUG_RECOVERY_MODE_
            printf("Block %lu is out-of-date: %lu vs %lu\n", rec->block_id, lsn, rec->b->lsn);
            #endif /* _DEBUG_RECOVERY_MODE_ */

            /* Replace block in database (but do not log this) */
            if (write_block(db->info->fd, db, rec->block_id, rec->b))
                break;
        }

        /* Free allocated memory */
        free_block(rec->b);
        free(rec);
    }

    if (rec) {
        /* Write failed */
        return 1;
    }

    return 0;
}