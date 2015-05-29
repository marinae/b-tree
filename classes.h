#ifndef __CLASSES_H__
#define __CLASSES_H__

#include "third_party/uthash.h"

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
//| Database header                                                            |
//+----------------------------------------------------------------------------+

typedef struct header {
	size_t block_size;
	size_t num_blocks;
	size_t first_node;
	size_t root_index;
	size_t max_key_size;
} header;

//+----------------------------------------------------------------------------+
//| Database info                                                              |
//+----------------------------------------------------------------------------+

typedef struct DB_info {
	int    fd;
    header *hdr;
    size_t bitmap_len;
    char   *bitmap;
} DB_info;

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
	/* For caching */
	size_t              id;
	struct block        *lru_next;
	struct block        *lru_prev;
	enum {DIRTY, CLEAN} status;
	/* For journaling */
	size_t lsn;
} block;

//+----------------------------------------------------------------------------+
//| Hashed pointer to the block b                                              |
//+----------------------------------------------------------------------------+

typedef struct hashed_pointer {
	block  *b;
	size_t id;
	UT_hash_handle hh;
} hashed_pointer;

//+----------------------------------------------------------------------------+
//| Cache                                                                      |
//+----------------------------------------------------------------------------+

typedef struct block_cache {
	size_t n_blocks;
	size_t max_blocks;
	/* Double-linked list of blocks */
	block *lru;
	block *lru_end;
	/* Quick look up by block ID */
	hashed_pointer *hashed_blocks;
} block_cache;

//+----------------------------------------------------------------------------+
//| Logger class                                                               |
//+----------------------------------------------------------------------------+

typedef struct Log {
	size_t log_file_len;
	char   *log_file_name;
	int    log_fd;
    size_t log_count;
} Log;

//+----------------------------------------------------------------------------+
//| Single record in log file                                                  |
//+----------------------------------------------------------------------------+

typedef struct Record {
	size_t lsn;
	size_t block_id;
	block  *b;
} Record;

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
    DB_info     *info;
    block       *root;
    block_cache *cache;
    Log         *logger;
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