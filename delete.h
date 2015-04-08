#ifndef __DELETE_H__
#define __DELETE_H__

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
//| Delete key from database                                                   |
//+----------------------------------------------------------------------------+

int f_delete(DB *db, DBT *key);
int delete_from(DB *db, block *x, size_t k, DBT *key);
int delete_here(DB *db, block *x, size_t k, DBT *key, size_t j);

#endif /* __DELETE_H__ */