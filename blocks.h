#ifndef __BLOCKS_H__
#define __BLOCKS_H__

#include "classes.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

//+----------------------------------------------------------------------------+
//| Work with blocks                                                           |
//+----------------------------------------------------------------------------+

int    write_block(DB *db, size_t k, block *b);
block  *read_block(DB *db, size_t k);
size_t find_empty_block(DB *db);
int    mark_block(DB *db, size_t k, bool state);

#endif /* __BLOCKS_H__ */