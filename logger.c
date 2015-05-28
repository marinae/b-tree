#include "logger.h"

//+----------------------------------------------------------------------------+
//| Open log file                                                              |
//+----------------------------------------------------------------------------+

Log *log_open(DB *db) {
	assert(db);
	/* Create file for write-ahead logging */
    int log_fd = open("wal", O_CREAT | O_RDWR, S_IRWXU);
    if (log_fd == -1)
        return NULL;
    /* Create structure */
    Log wal = {
    	.log_fd    = log_fd,
    	.log_count = 0
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
    printf("0x%08x\t%lu\t%lu\t\n", RECORD_MARKER, record->lsn, record->block_id);
    #endif /* _DEBUG_WAL_MODE_ */
    if (-1 == write(db->logger->log_fd, (void *)&RECORD_MARKER, sizeof(RECORD_MARKER)))
        return 1;
    /* Write LSN */
    if (-1 == write(db->logger->log_fd, (void *)&record->lsn, sizeof(record->lsn)))
        return 1;
    /* Write block ID */
    if (-1 == write(db->logger->log_fd, (void *)&record->block_id, sizeof(record->block_id)))
        return 1;
    /* Write block */
    if (-1 == write_block(db->logger->log_fd, db, record->block_id, record->b))
        return 1;
    ++db->logger->log_count;
    /* Write checkpoint? */
    if (db->logger->log_count % CP_FREQUENCY == 0) {
        #ifdef _DEBUG_WAL_MODE_
        printf("0x%08x\n", CHECKPOINT);
        #endif /* _DEBUG_WAL_MODE_ */
        if (-1 == write(db->logger->log_fd, (void *)&CHECKPOINT, sizeof(CHECKPOINT)))
            return 1;
    }
    return 0;
}

//+----------------------------------------------------------------------------+
//| Seek previous checkpoint in WAL                                            |
//+----------------------------------------------------------------------------+

void log_seek(DB *db) {
    //
}

//+----------------------------------------------------------------------------+
//| Read next record from WAL                                                  |
//+----------------------------------------------------------------------------+

Record *log_read_next(DB *db) {
    //
    return NULL;
}