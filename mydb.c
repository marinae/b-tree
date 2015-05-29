#include "mydb.h"

//+----------------------------------------------------------------------------+
//| Open database                                                              |
//+----------------------------------------------------------------------------+

DB *dbopen(char *file) {
    printf("Open database\n");
    /* Check params */
    assert(file);

    /* Allocate memory for structure */
    DB *db = (DB *)calloc(1, sizeof(DB));
    /* Open file with database */
    int fd = open(file, O_RDWR);
    if (fd == -1) {
        free(db);
        return NULL;
    }

    // TODO: fill db parameters

    /* Fill private API */
    db->_write_block      = c_write_block;
    db->_read_block       = c_read_block;
    db->_find_empty_block = find_empty_block;
    db->_mark_block       = mark_block;
    /* Fill public API */
    db->_close  = f_close;
    db->_delete = f_delete;
    db->_insert = f_insert;
    db->_select = f_select;
    db->_sync   = f_sync;

    /* Scan WAL and synchronize */
    db->_sync(db);

    return db;
}

//+----------------------------------------------------------------------------+
//| Create database                                                            |
//+----------------------------------------------------------------------------+

DB *dbcreate(char *file, DBC conf) {  
    printf("Create database\n");
    /* Check params */
    assert(file);

    /* Allocate memory for structure */
    DB *db = (DB *)calloc(1, sizeof(DB));

    /* Create file for database */
    int fd = open(file, O_CREAT | O_RDWR, S_IRWXU);
    if (fd == -1) {
        free(db);
        return NULL;
    }

    /* Fill root info */
    db->root = (block *)calloc(1, sizeof(block));

    /* Fill private API */
    db->_write_block      = c_write_block;
    db->_read_block       = c_read_block;
    db->_find_empty_block = find_empty_block;
    db->_mark_block       = mark_block;

    /* Fill public API */
    db->_close  = f_close;
    db->_delete = f_delete;
    db->_insert = f_insert;
    db->_select = f_select;
    db->_sync   = f_sync;

    int success = 1;

    /* Fill DB info + cache + log info */
    if (success) {
        if (fill_db_info(conf, fd, &db->info))
            success = 0;
    }
    assert(db->info);
    if (success) {
        if (create_cache(conf.cache_size / conf.page_size, &db->cache))
            success = 0;
    }
    if (success) {
        if (create_log(db))
            success = 0;
    }
    if (success) {
        if (write_headers(db))
            success = 0;
    }

    if (success) {
        /* Print DB parameters */
        printf("Block size:\t\t%lu bytes\n", db->info->hdr->block_size);
        printf("Num blocks:\t\t%lu\n", db->info->hdr->num_blocks);
        printf("First data block:\t%lu\n", db->info->hdr->first_node);
        printf("Max cache size:\t\t%lu blocks\n", db->cache->max_blocks);
        printf("Log file:\t\t%s\n", db->logger->log_file_name);

    } else {
        db->_close(db);
        return NULL;
    }

    return db;
}

//+----------------------------------------------------------------------------+
//| Fill header object                                                         |
//+----------------------------------------------------------------------------+

int fill_header(size_t block_size, size_t num_blocks, size_t bitmap_len, header **hdr) {
    assert(!(*hdr));

    /* Fill header */
    header tmp_hdr = {
        .block_size   = block_size,
        .num_blocks   = num_blocks,
        .first_node   = 0,
        .root_index   = 0,
        .max_key_size = 0
    };

    tmp_hdr.first_node =  sizeof(tmp_hdr) + 2 * sizeof(size_t) + bitmap_len + sizeof(WAL_FILE);
    tmp_hdr.root_index =  tmp_hdr.first_node;

    /* Fill header info */
    *hdr = (header *)calloc(1, sizeof(header));
    memcpy(*hdr, &tmp_hdr, sizeof(tmp_hdr));

    return 0;
}

//+----------------------------------------------------------------------------+
//| Fill DB info                                                               |
//+----------------------------------------------------------------------------+

int fill_db_info(DBC conf, int fd, DB_info **info) {
    assert(!(*info));

    /* Compute parameters */
    size_t num_blocks    = conf.db_size / conf.page_size;
    size_t bitmap_len    = ceil(num_blocks / 8);
    char   *bitmap       = (char *)calloc(bitmap_len, sizeof(char));

    /* Check last bitmap byte */
    size_t diff =  num_blocks - bitmap_len * 8;
    if (diff > 0) {
        bitmap[bitmap_len - 1] = 255 >> diff;
    }

    /* Fill DB info */
    DB_info tmp_info = {
        .fd            = fd,
        .hdr           = NULL,
        .bitmap_len    = bitmap_len,
        .bitmap        = bitmap
    };
    fill_header(conf.page_size, num_blocks, bitmap_len, &tmp_info.hdr);

    /* Fill info */
    *info = (DB_info *)calloc(1, sizeof(tmp_info));
    memcpy(*info, &tmp_info, sizeof(tmp_info));

    return 0;
}

//+----------------------------------------------------------------------------+
//| Create block cache                                                         |
//+----------------------------------------------------------------------------+

int create_cache(size_t max_cached, block_cache **cache) {
    assert(!(*cache));

    /* Fill cache info */
    block_cache tmp_cache = {
        .n_blocks      = 0,
        .max_blocks    = max_cached,
        .lru           = NULL,
        .lru_end       = NULL,
        .hashed_blocks = NULL
    };
    *cache = (block_cache *)calloc(1, sizeof(tmp_cache));
    memcpy(*cache, &tmp_cache, sizeof(tmp_cache));

    return 0;
}

//+----------------------------------------------------------------------------+
//| Create log                                                                 |
//+----------------------------------------------------------------------------+

int create_log(DB *db) {
    assert(db);

    /* Create WAL */
    Log *logger = log_open(db);
    if (!logger)
        return 1;

    db->logger = logger;

    return 0;
}

//+----------------------------------------------------------------------------+
//| Write headers to file                                                      |
//+----------------------------------------------------------------------------+

int write_headers(DB *db) {
    assert(db);
    assert(db->info);
    assert(db->info->hdr);
    assert(db->logger);

    ssize_t written   = 0;
    ssize_t sum_bytes = 0;

    /* Write file header */
    written += write(db->info->fd, (void*)db->info->hdr, sizeof(*(db->info->hdr)));
    sum_bytes += sizeof(*(db->info->hdr));

    /* Write bitmap */
    written += write(db->info->fd, (void*)&db->info->bitmap_len, sizeof(db->info->bitmap_len));
    written += write(db->info->fd, (void*)db->info->bitmap, db->info->bitmap_len);
    sum_bytes += sizeof(db->info->bitmap_len) + db->info->bitmap_len;

    /* Write WAL filename */
    written += write(db->info->fd, (void*)&db->logger->log_file_len, sizeof(db->logger->log_file_len));
    written += write(db->info->fd, (void*)db->logger->log_file_name, db->logger->log_file_len);
    sum_bytes += sizeof(db->logger->log_file_len) + db->logger->log_file_len;

    if (written != sum_bytes)
        return 1;

    return 0;
}

//+----------------------------------------------------------------------------+
//| Close database (call method)                                               |
//+----------------------------------------------------------------------------+

int db_close(struct DB *db) {
	return db->_close(db);
}

//+----------------------------------------------------------------------------+
//| Flush database (call method)                                               |
//+----------------------------------------------------------------------------+

int db_flush(struct DB *db) {
    return db->_sync(db);
}

//+----------------------------------------------------------------------------+
//| Delete key (call method)                                                   |
//+----------------------------------------------------------------------------+

int db_delete(struct DB *db, void *key, size_t key_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	return db->_delete(db, &keyt);
}

//+----------------------------------------------------------------------------+
//| Select key-value (call method)                                             |
//+----------------------------------------------------------------------------+

int db_select(struct DB *db, void *key, size_t key_len,
	   void **val, size_t *val_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	struct DBT valt = {0, 0};
	int rc = db->_select(db, &keyt, &valt);
	*val = valt.data;
	*val_len = valt.size;
	return rc;
}

//+----------------------------------------------------------------------------+
//| Insert key-value (call method)                                             |
//+----------------------------------------------------------------------------+

int db_insert(struct DB *db, void *key, size_t key_len,
	   void *val, size_t val_len) {
	struct DBT keyt = {
		.data = key,
		.size = key_len
	};
	struct DBT valt = {
		.data = val,
		.size = val_len
	};
	return db->_insert(db, &keyt, &valt);
}
