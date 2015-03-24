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

int f_insert(DB *db, DBT *key, DBT *value);
int insert_nonfull(DB *db, block *b, size_t k, DBT *key, DBT *value);
int split_child(DB *db, block *b, size_t b_block, size_t index);

#endif /* __INSERT_H__ */