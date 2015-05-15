#ifndef __CACHE_H__
#define __CACHE_H__

#include "classes.h"
#include "helpful.h"
#include "blocks.h"

#include <stdlib.h>

//+----------------------------------------------------------------------------+
//| Work with blocks and cache                                                 |
//+----------------------------------------------------------------------------+

int    c_write_block(DB *db, size_t k, block *b_new);
block* c_read_block(DB *db, size_t k);
block* search_in_cache(hashed_pointer *htable, size_t k);
block  *add_to_cache(DB *db, size_t k);
int    push_front(block_cache *cache, block *b); 
int    pop_back(DB *db, block_cache *cache);

#endif /* __CACHE_H__ */