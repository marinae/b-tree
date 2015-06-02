#ifndef __MYDB_H__
#define __MYDB_H__

#define _DEBUG_DB_MODE_
//#define _BAD_TEST_

#include "classes.h"
#include "blocks.h"
#include "cache.h"
#include "insert.h"
#include "select.h"
#include "delete.h"
#include "database.h"

#include <assert.h>

//+----------------------------------------------------------------------------+
//| DB format:                                                                 |
//| - header:                                                                  |
//|   - block_size      (size_t)                                               |
//|   - num_blocks      (size_t)                                               |
//|   - first_node      (size_t)                                               |
//|   - root_index      (size_t)                                               |
//|   - max_key_size    (size_t)                                               |
//| - bitmap:                                                                  |
//|   - bitmap length   (size_t)                                               |
//|   - bitmap          (bitmap length)                                        |
//| - WAL:                                                                     |
//|   - filename length (size_t)                                               |
//|   - filename        (filename length)                                      |
//+----------------------------------------------------------------------------+

//+----------------------------------------------------------------------------+
//| Functions for initializing DB                                              |
//+----------------------------------------------------------------------------+

DB *dbopen(char *file, DBC conf);
DB *dbcreate(char *file, DBC conf);

//+----------------------------------------------------------------------------+
//| Helpful functions                                                          |
//+----------------------------------------------------------------------------+

int fill_header(size_t block_size, size_t num_blocks, size_t bitmap_len, header **hdr);
int fill_db_info(DBC conf, int fd, DB_info **info);
int create_cache(size_t max_cached, block_cache **cache);
int create_log(DB *db, char *log_file_name);
int write_headers(DB *db);
DB_info *read_headers(int fd);
char *read_log_params(int fd);

//+----------------------------------------------------------------------------+
//| Functions calling DB API                                                   |
//+----------------------------------------------------------------------------+

int db_close(DB *db);
int db_flush(DB *db);
int db_delete(DB *db, void *key, size_t key_len);
int db_select(DB *db, void *key, size_t key_len, void **val, size_t *val_len);
int db_insert(DB *db, void *key, size_t key_len, void *val, size_t val_len);

#endif /* __MYDB_H__ */
