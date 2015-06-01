#ifndef __SELECT_H__
#define __SELECT_H__

//#define _DEBUG_SELECT_MODE_

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
//| Select key from database                                                   |
//+----------------------------------------------------------------------------+

int f_select(DB *db, DBT *key, DBT *value);
int key_search(DB *db, block *cur, DBT *key, DBT *value);
int alloc_value(item *it, DBT *value);

#endif /* __SELECT_H__ */