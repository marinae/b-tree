#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "classes.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

//+----------------------------------------------------------------------------+
//| Functions for filling API                                                  |
//+----------------------------------------------------------------------------+

int f_close(DB *db);
int f_delete(DB *db, DBT *key);
int f_insert(DB *db, DBT *key, DBT *value);
int f_select(DB *db, DBT *key, DBT *value);
int f_sync(DB *db);

//+----------------------------------------------------------------------------+
//| Helpful functions                                                          |
//+----------------------------------------------------------------------------+

int    insert_nonfull(DB *db, block *b, size_t k, DBT *key, DBT *value);
size_t free_space(block *b, size_t block_size);

#endif /* __DATABASE_H__ */