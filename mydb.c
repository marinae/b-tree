#include "mydb.h"

//+----------------------------------------------------------------------------+
//| Open database                                                              |
//+----------------------------------------------------------------------------+

DB *dbopen(char *file, DBC conf) {
    /* Check params */
    assert(file);

    /* Open file with database */
    int fd = open(file, O_RDWR);
    if (fd == -1)
        return NULL;

    /* Allocate memory for structure */
    DB *db = (DB *)calloc(1, sizeof(DB));

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
    char *log_file_name;

    /* Read file headers + open log file + create cache */
    if (success) {
        db->info = read_headers(fd);
        if (!db->info)
            success = 0;
    }
    if (success) {
        log_file_name = read_log_params(fd);
        if (!log_file_name)
            success = 0;
    }
    if (success) {
        if (create_log(db, log_file_name))
            success = 0;
    }
    if (success) {
        if (create_cache(conf.cache_size / conf.page_size, &db->cache))
            success = 0;
    }
    /* Read root block */
    if (success) {
        db->root = read_block(fd, db, db->info->hdr->root_index);
        if (!db->root)
            success = 0;
    }
    /* Scan WAL and synchronize */
    /*if (success) {
        if (db->_sync(db))
            success = 0;
    }*/
    
    if (success) {
        /* Print DB parameters */
        #ifdef _DEBUG_DB_MODE_
        printf(">> Database parameters <<\n");
        printf("Block size:\t\t%lu bytes\n", db->info->hdr->block_size);
        printf("Num blocks:\t\t%lu\n", db->info->hdr->num_blocks);
        printf("First data block:\t%lu\n", db->info->hdr->first_node);
        printf("Root index:\t\t%lu\n", db->info->hdr->root_index);
        printf("Bitmap offset:\t\t%lu\n", db->info->hdr->bitmap_offset);
        printf("Max cache size:\t\t%lu blocks\n", db->cache->max_blocks);
        printf("Log file:\t\t%s\n", db->logger->log_file_name);
        printf(">> Database successfully opened <<\n"); 
        #endif /* _DEBUG_DB_MODE_ */

    } else {
        db->_close(db);
        return NULL;
    }
    return db;
}

//+----------------------------------------------------------------------------+
//| Create database                                                            |
//+----------------------------------------------------------------------------+

DB *dbcreate(char *file, DBC conf) { 
    /* Check params */
    assert(file);

    /* Create file for database */
    int fd = open(file, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
    /* Error -> file already exists? */
    if (fd == -1)
        return dbopen(file, conf);

    /* Allocate memory for structure */
    DB *db = (DB *)calloc(1, sizeof(DB));

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
    if (success) {
        if (create_cache(conf.cache_size / conf.page_size, &db->cache))
            success = 0;
    }
    if (success) {
        char *log_file_name = (char *)calloc(sizeof(WAL_FILE), sizeof(char));
        memcpy(log_file_name, (void *)WAL_FILE, sizeof(WAL_FILE));

        if (create_log(db, log_file_name))
            success = 0;
    }
    if (success) {
        if (write_headers(db))
            success = 0;
    }

    if (success) {
        /* Print DB parameters */
        #ifdef _DEBUG_DB_MODE_
        printf(">> Database parameters <<\n");
        printf("Block size:\t\t%lu bytes\n", db->info->hdr->block_size);
        printf("Num blocks:\t\t%lu\n", db->info->hdr->num_blocks);
        printf("First data block:\t%lu\n", db->info->hdr->first_node);
        printf("Root index:\t\t%lu\n", db->info->hdr->root_index);
        printf("Bitmap offset:\t\t%lu\n", db->info->hdr->bitmap_offset);
        printf("Max cache size:\t\t%lu blocks\n", db->cache->max_blocks);
        printf("Log file:\t\t%s\n", db->logger->log_file_name);
        printf(">> Database successfully created <<\n"); 
        #endif /* _DEBUG_DB_MODE_ */

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
        .block_size    = block_size,
        .num_blocks    = num_blocks,
        .first_node    = 0,
        .root_index    = 0,
        .bitmap_offset = sizeof(header) + sizeof(size_t),
        .max_key_size  = 0
    };

    size_t offset = sizeof(tmp_hdr) + 2 * sizeof(size_t) + bitmap_len + sizeof(WAL_FILE);

    tmp_hdr.first_node =  ceil((double)offset / tmp_hdr.block_size);
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
    size_t bitmap_len    = ceil((double)num_blocks / 8);
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

int create_log(DB *db, char *log_file_name) {
    assert(db);

    /* Create WAL */
    Log *logger = log_open(db, log_file_name);
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
//| Read DB info from file                                                     |
//+----------------------------------------------------------------------------+

DB_info *read_headers(int fd) {

    header *hdr = (header *)calloc(1, sizeof(header));
    ssize_t read_bytes = 0;
    ssize_t sum_bytes  = 0;

    /* Read file header */
    read_bytes += read(fd, (void*)hdr, sizeof(header));
    sum_bytes  += sizeof(header);

    /* Read bitmap length */
    size_t bitmap_len;
    read_bytes += read(fd, (void*)&bitmap_len, sizeof(bitmap_len));
    sum_bytes  += sizeof(bitmap_len);

    /* Read bitmap */
    char *bitmap = (char *)calloc(bitmap_len, sizeof(char));
    read_bytes += read(fd, (void*)bitmap, bitmap_len);
    sum_bytes  += bitmap_len;

    if (read_bytes != sum_bytes) {
        free(hdr);
        free(bitmap);
        return NULL;
    }

    /* Fill DB info */
    DB_info tmp_info = {
        .fd         = fd,
        .hdr        = hdr,
        .bitmap_len = bitmap_len,
        .bitmap     = bitmap
    };
    DB_info *info = (DB_info *)calloc(1, sizeof(DB_info));
    memcpy(info, &tmp_info, sizeof(tmp_info));

    return info;
}

//+----------------------------------------------------------------------------+
//| Read log file params from DB file                                          |
//+----------------------------------------------------------------------------+

char *read_log_params(int fd) {

    ssize_t read_bytes = 0;
    ssize_t sum_bytes  = 0;

    /* Read WAL file name length */
    size_t log_file_len;
    read_bytes += read(fd, (void*)&log_file_len, sizeof(log_file_len));
    sum_bytes  += sizeof(log_file_len);

    /* Read WAL file name */
    char *log_file_name = (char *)calloc(log_file_len, sizeof(char));
    read_bytes += read(fd, (void*)log_file_name, log_file_len);
    sum_bytes  += log_file_len;

    if (read_bytes != sum_bytes) {
        free(log_file_name);
        return NULL;
    }

    return log_file_name;
}

//+----------------------------------------------------------------------------+
//| Close database (call method)                                               |
//+----------------------------------------------------------------------------+

int db_close(struct DB *db) {
    #ifdef _DEBUG_DB_MODE_
    printf(">> Closing database <<\n"); 
    #endif /* _DEBUG_DB_MODE_ */

	return db->_close(db);
}

//+----------------------------------------------------------------------------+
//| Flush database (call method)                                               |
//+----------------------------------------------------------------------------+

int db_flush(struct DB *db) {
    #ifdef _DEBUG_DB_MODE_
    printf(">> Synchronizing database <<\n"); 
    #endif /* _DEBUG_DB_MODE_ */

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
