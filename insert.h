#ifndef __INSERT_H__
#define __INSERT_H__

#include "classes.h"
#include "helpful.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

//+----------------------------------------------------------------------------+
//| Insert key into database                                                   |
//+----------------------------------------------------------------------------+

int    f_insert(DB *db, DBT *key, DBT *value);
int    insert_nonfull(DB *db, block *x, size_t k, DBT *key, DBT *value);
int    split_child(DB *db, block *x, size_t x_block, size_t child);
int    insert_item(DB *db, block *x, size_t k, DBT *key, DBT *value, size_t chd);
block  *split_node(block *x, size_t from, size_t to);
size_t find_child(block *x, DBT *key);

#endif /* __INSERT_H__ */