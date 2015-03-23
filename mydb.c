#include "mydb.h"

// TODO: open database

//+----------------------------------------------------------------------------+
//| Open database                                                              |
//+----------------------------------------------------------------------------+

DB *dbopen(char *file) {
    /* Check params */
    assert(file);

    DB *db = (DB *)calloc(1, sizeof(DB));
    return db;
}

//+----------------------------------------------------------------------------+
//| Create database                                                            |
//+----------------------------------------------------------------------------+

DB *dbcreate(char *file, DBC conf) {  
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

    /* Compute parameters */
    size_t num_blocks    = conf.db_size / conf.page_size;
    size_t bitmap_len    = ceil(num_blocks / 8);
    size_t bitmap_blocks = ceil(bitmap_len / conf.page_size);
    char   *bitmap       = (char *)calloc(bitmap_len, sizeof(char));

    /* Check last bitmap byte */
    size_t diff =  num_blocks - bitmap_len * 8;
    if (diff > 0) {
        bitmap[bitmap_len - 1] = 255 >> diff;
    }

    /* Fill DB info */
    DB_info info = {
        .fd            = fd,
        .block_size    = conf.page_size,
        .num_blocks    = num_blocks,
        .bitmap_len    = bitmap_len,
        .bitmap_blocks = bitmap_blocks,
        .bitmap        = bitmap,
        .first_node    = bitmap_blocks + 1,
        .root_index    = bitmap_blocks + 1
    };
    db->info = (DB_info *)calloc(1, sizeof(DB_info));
    memcpy(db->info, &info, sizeof(info));

    /* Print DB parameters */
    printf("fd = %d\n",             db->info->fd);
    printf("block_size    = %lu\n", db->info->block_size);
    printf("num_blocks    = %lu\n", db->info->num_blocks);
    printf("bitmap_len    = %lu\n", db->info->bitmap_len);
    printf("bitmap_blocks = %lu\n", db->info->bitmap_blocks);
    printf("first_node    = %lu\n", db->info->first_node);
    printf("root_index    = %lu\n", db->info->root_index);
    
    /* Write file header */
    header hr = {
        .block_size = db->info->block_size,
        .num_blocks = db->info->num_blocks,
        .root_index = db->info->root_index
    };
    ssize_t written = write(info.fd, (void*)&hr, sizeof(hr));
    if (written != sizeof(hr)) {
        free(db->info->bitmap);
        free(db->info);
        free(db);
        return NULL;
    }

    /* Write bitmap */
    lseek(fd, db->info->block_size, SEEK_SET);
    written = write(fd, (void*)db->info->bitmap, db->info->bitmap_len);
    if (written != db->info->bitmap_len) {
        free(db->info->bitmap);
        free(db->info);
        free(db);
        return NULL;
    }

    /* Fill root info */
    block root = {
        .num_keys     = 0,
        .items        = NULL,
        .num_children = 0,
        .children     = NULL
    };
    db->root = (block *)calloc(1, sizeof(block));
    memcpy(db->root, &root, sizeof(root));

    /* Fill private API */
    db->_write_block      = write_block;
    db->_read_block       = read_block;
    db->_find_empty_block = find_empty_block;
    db->_mark_block       = mark_block;

    /* Fill public API */
    db->_close  = f_close;
    db->_delete = f_delete;
    db->_insert = f_insert;
    db->_select = f_select;
    db->_sync   = f_sync;

    return db;
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
