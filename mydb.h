#ifndef __MYDB_H__
#define __MYDB_H__

#include "classes.h"
#include "blocks.h"
#include "cache.h"
#include "insert.h"
#include "select.h"
#include "delete.h"
#include "database.h"
#include <assert.h>

//+----------------------------------------------------------------------------+
//| Functions for initializing DB                                              |
//+----------------------------------------------------------------------------+

DB *dbopen(char *file);
DB *dbcreate(char *file, DBC conf);

//+----------------------------------------------------------------------------+
//| Functions calling DB API                                                   |
//+----------------------------------------------------------------------------+

int db_close(DB *db);
int db_flush(DB *db);
int db_delete(DB *db, void *key, size_t key_len);
int db_select(DB *db, void *key, size_t key_len, void **val, size_t *val_len);
int db_insert(DB *db, void *key, size_t key_len, void *val, size_t val_len);

#endif /* __MYDB_H__ */
