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

int f_close(struct DB *db);
int f_delete(struct DB *db, struct DBT *key);
int f_insert(struct DB *db, struct DBT *key, struct DBT *value);
int f_select(struct DB *db, struct DBT *key, struct DBT *value);
int f_sync(struct DB *db);

#endif /* __DATABASE_H__ */