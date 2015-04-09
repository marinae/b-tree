#ifndef __HELPFUL_H__
#define __HELPFUL_H__

#include "classes.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

//+----------------------------------------------------------------------------+
//| Helpful functions                                                          |
//+----------------------------------------------------------------------------+

size_t free_space(block *b, size_t block_size);
int    compare_keys(DBT *key1, DBT *key2);
size_t contains_key(block *x, DBT *key);
item   *create_item(DBT *key, DBT *value);
int    free_block(block *b);
int    free_item(item *it);
bool   enough_mem(DB *db, block *b, DBT *key, DBT *value);
int    make_root(DB *db, size_t k);
size_t need_memory(block *x);
int    print_tree(DB *db, block *cur);
size_t find_child(block *x, DBT *key);

#endif /* __HELPFUL_H__ */