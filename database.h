#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "classes.h"
#include "cache.h"
#include "helpful.h"

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
int f_sync(DB *db);

#endif /* __DATABASE_H__ */