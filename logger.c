#include "logger.h"

//+----------------------------------------------------------------------------+
//| Open log file                                                              |
//+----------------------------------------------------------------------------+

Log *log_open(DB *db, char *log_file_name) {
	assert(db);
	/* Create file for write-ahead logging */
    int log_fd = open(log_file_name, O_CREAT | O_RDWR, S_IRWXU);
    if (log_fd == -1)
        return NULL;
    /* Create structure */
    Log wal = {
        .log_file_len  = sizeof(log_file_name),
        .log_file_name = log_file_name,
    	.log_fd        = log_fd,
    	.log_count     = 0
    };

    /* Copy to pointer */
    Log *logger = (Log *)calloc(1, sizeof(Log));
    memcpy(logger, &wal, sizeof(wal));

    return logger;
}

//+----------------------------------------------------------------------------+
//| Close log file                                                             |
//+----------------------------------------------------------------------------+

void log_close(DB *db) {
	assert(db);
    close(db->logger->log_fd);
}

//+----------------------------------------------------------------------------+
//| Write record to WAL                                                        |
//+----------------------------------------------------------------------------+

int log_write(DB *db, Record *record) {
    assert(db && db->logger && record);
    /* Write marker */
    #ifdef _DEBUG_WAL_MODE_
    printf("0x%08x\t%lu\t%lu\t\n", RECORD_MARKER, record->b->lsn, record->block_id);
    #endif /* _DEBUG_WAL_MODE_ */
    if (-1 == write(db->logger->log_fd, (void *)&RECORD_MARKER, sizeof(RECORD_MARKER)))
        return 1;
    /* Write block ID */
    if (-1 == write(db->logger->log_fd, (void *)&record->block_id, sizeof(record->block_id)))
        return 1;
    /* Write block */
    if (-1 == write_block(db->logger->log_fd, db, record->block_id, record->b))
        return 1;
    ++db->logger->log_count;
    /* Write checkpoint */
    if (db->logger->log_count % CP_FREQUENCY == 0) {
        size_t lsn = db->logger->log_count;
        /* Flush cache */
        if (flush_cache(db, lsn))
            return 1;
        /* Print checkpoint */
        #ifdef _DEBUG_WAL_MODE_
        printf("0x%08x\t%lu\n", CHECKPOINT, lsn);
        #endif /* _DEBUG_WAL_MODE_ */
        if (-1 == write(db->logger->log_fd, (void *)&CHECKPOINT, sizeof(CHECKPOINT)))
            return 1;
        if (-1 == write(db->logger->log_fd, (void *)&lsn, sizeof(lsn)))
            return 1;
        ++db->logger->log_count;
    }
    return 0;
}

//+----------------------------------------------------------------------------+
//| Seek previous checkpoint in WAL                                            |
//+----------------------------------------------------------------------------+

void log_seek(DB *db) {
    // TODO: seek checkpoint

    #ifdef _DEBUG_WAL_MODE_
    printf(">> Scanning WAL <<\n");
    #endif /* _DEBUG_WAL_MODE_ */

    lseek(db->logger->log_fd, 0, SEEK_SET);
}

//+----------------------------------------------------------------------------+
//| Read next record from WAL                                                  |
//+----------------------------------------------------------------------------+

Record *log_read_next(DB *db) {
    assert(db && db->logger);

    size_t offset;
    int    marker;

    /* Scan WAL */
    while (read(db->logger->log_fd, &marker, sizeof(int)) > 0) {

        if (marker == RECORD_MARKER) {
            /* Next record found */
            size_t block_id;

            /* Read block ID and block itself */
            read(db->logger->log_fd, &block_id, sizeof(block_id));
            block *b = read_block(db->logger->log_fd, db, block_id);

            if (!b)
                break;

            /* Allocate memory for record */
            Record *rec = (Record *)calloc(1, sizeof(Record));

            /* Copy attributes */
            rec->block_id = block_id;
            rec->b        = b;

            #ifdef _DEBUG_WAL_MODE_
            printf("Record #%lu found: block %lu\n", b->lsn, block_id);
            #endif /* _DEBUG_WAL_MODE_ */

            return rec;
        }

        lseek(db->logger->log_fd, 1 - sizeof(int), SEEK_CUR);
    }

    #ifdef _DEBUG_WAL_MODE_
    printf("Nothing found\n");
    #endif /* _DEBUG_WAL_MODE_ */

    return NULL;
}