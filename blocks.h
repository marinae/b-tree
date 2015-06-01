#ifndef __BLOCKS_H__
#define __BLOCKS_H__

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
//| Work with blocks                                                           |
//+----------------------------------------------------------------------------+

int    write_block(int fd, DB *db, size_t k, block *b);
block  *read_block(int fd, DB *db, size_t k);
size_t find_empty_block(DB *db);
int    mark_block(DB *db, size_t k, bool state);
block  *copy_block(block *b);
int    copy_block_to(block *b, block *b_new);
int    blockcpy(block *b, block *b_copy);
size_t get_lsn(DB *db, size_t k);

#endif /* __BLOCKS_H__ */