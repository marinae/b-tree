#ifndef __CLASSES_H__
#define __CLASSES_H__

#include <stddef.h>
#include <stdbool.h>

//+----------------------------------------------------------------------------+
//| Item and size                                                              |
//+----------------------------------------------------------------------------+

typedef struct DBT {
	void   *data;
	size_t size;
} DBT;

//+----------------------------------------------------------------------------+
//| Database info                                                              |
//+----------------------------------------------------------------------------+

typedef struct DB_info {
	int    fd;
    size_t block_size;
    size_t num_blocks;
    size_t bitmap_len;
    size_t bitmap_blocks;
    char   *bitmap;
    size_t first_node;
    size_t root_index;
} DB_info;

//+----------------------------------------------------------------------------+
//| Database header                                                            |
//+----------------------------------------------------------------------------+

typedef struct header {
	size_t block_size;
	size_t num_blocks;
	size_t root_index;
} header;

//+----------------------------------------------------------------------------+
//| Database item                                                              |
//+----------------------------------------------------------------------------+

typedef struct item {
	DBT *key;
	DBT *value;
} item;

//+----------------------------------------------------------------------------+
//| Database block (node)                                                      |
//+----------------------------------------------------------------------------+

typedef struct block {
	size_t num_keys;
	item   **items;
	size_t num_children;
	size_t *children;
} block;

//+----------------------------------------------------------------------------+
//| Database API                                                               |
//+----------------------------------------------------------------------------+

typedef struct DB {
	/* Public API */
	/* Returns 0 on OK, -1 on Error */
	int (*_close)(struct DB *db);
	int (*_delete)(struct DB *db, DBT *key);
	int (*_insert)(struct DB *db, DBT *key, DBT *value);
	/* * * * * * * * * * * * * *
	 * Returns malloc'ed data into 'struct DBT *value'.
	 * Caller must free value->data. 'struct DBT *value' must be alloced in
	 * caller.
	 * * * * * * * * * * * * * */
	int (*_select)(struct DB *db, DBT *key, DBT *value);
	/* Sync cached pages with disk
	 * */
	int (*_sync)(struct DB *db);
	/* For future uses - sync cached pages with disk
	 * int (*sync)(const struct DB *db)
	 * */
    
	/* Private API */
	/* Returns 0 on OK, -1 on Error */
	int    (*_write_block)(struct DB *db, size_t k, struct block *b);
	block  *(*_read_block)(struct DB *db, size_t k);
	size_t (*_find_empty_block)(struct DB *db);
	int    (*_mark_block)(struct DB *db, size_t k, bool state);
    
    /* DB parameters: file descriptor, node size, root offset, etc. */
    DB_info *info;
    block   *root;
    size_t  max_key_size;
} DB; /* Need for supporting multiple backends (HASH/BTREE) */

//+----------------------------------------------------------------------------+
//| Database configuration                                                     |
//+----------------------------------------------------------------------------+

typedef struct DBC {
	/* Maximum on-disk file size (512MB) */
	size_t db_size;
	/* Page (node/data) size (4KB) */
	size_t page_size;
	/* Maximum cached memory size (16MB) */
	size_t cache_size;
} DBC;

#endif /* __CLASSES_H__ */