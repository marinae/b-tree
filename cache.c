#include "cache.h"

//+----------------------------------------------------------------------------+
//| Write block (+cache)                                                       |
//+----------------------------------------------------------------------------+

int c_write_block(DB *db, size_t k, block *b_new) {
	assert(db && b_new);
	/* Search this block in cache */
	block *b = search_in_cache(db->cache->hashed_blocks, k);
	if (b != NULL) {
		/* Found -> update block and status */
		if (b->status == CLEAN) {
			b->status = DIRTY;
			//printf("Block %lu marked as DIRTY\n", k);
		}
		// TODO: move block to the first position in cache
		return copy_block_to(b, b_new);

	} else {
		return write_block(db, k, b_new);
	}
}

//+----------------------------------------------------------------------------+
//| Read block (+cache)                                                        |
//+----------------------------------------------------------------------------+

block* c_read_block(DB *db, size_t k) {
	assert(db && db->cache);
	/* Search this block in cache */
	block *b = search_in_cache(db->cache->hashed_blocks, k);
	if (b != NULL) {
		/* Found -> return its copy */
		/*if (b->status == CLEAN)
			printf("Block %lu found in cache, status = CLEAN\n", k);
		else
			printf("Block %lu found in cache, status = DIRTY\n", k);*/
		return copy_block(b);

	} else {
		/* Not found -> read from disc and add to cache */
		return add_to_cache(db, k);
	}
}

//+----------------------------------------------------------------------------+
//| Search block k in hash table                                               |
//+----------------------------------------------------------------------------+

block* search_in_cache(hashed_pointer *htable, size_t k) {
	hashed_pointer *hpointer;
	HASH_FIND_INT(htable, &k, hpointer);
	if (hpointer)
		return hpointer->b;
	else
		return NULL;
}

//+----------------------------------------------------------------------------+
//| Read block from disc and add it to cache                                   |
//+----------------------------------------------------------------------------+

block *add_to_cache(DB *db, size_t k) {
	assert(db && db->cache);
	/* Check if cache is full */
	if (db->cache->n_blocks == db->cache->max_blocks) {
		/* Pop last used block from cache (last item in list) */
		if (db->cache->max_blocks == 0) {
			/* No caching */
			return read_block(db, k);
		} else {
			pop_back(db, db->cache);
		}
	}
	assert(db->cache->n_blocks < db->cache->max_blocks);
	/* Read block from disc */
	block *b = read_block(db, k);
	block *b_copy = copy_block(b);
	b_copy->id = k;
	/* Create pointer to this block */
	hashed_pointer *hpointer = (hashed_pointer *)calloc(1, sizeof(hashed_pointer));
	hpointer->b = b_copy;
	hpointer->id = k;
	/* Add to hash table */
	HASH_ADD_INT(db->cache->hashed_blocks, id, hpointer);
	/* Prepend to block list */
	b_copy->status = CLEAN;
	push_front(db->cache, b_copy);

	//printf("Block %lu added to cache, free space = %lu\n", k, db->cache->max_blocks - db->cache->n_blocks);

	return b;
}

//+----------------------------------------------------------------------------+
//| Prepend block to double-ended list                                         |
//+----------------------------------------------------------------------------+

int push_front(block_cache *cache, block *b) {
	assert(cache && cache->n_blocks < cache->max_blocks);
	/* Increase number of blocks in cache */
	if (cache->n_blocks == 0) {
		/* No blocks in LRU */
		cache->lru = b;
	} else {
		/* Set previous block as b for first block in LRU */
		block *old_root = cache->lru;
		old_root->lru_prev = b;
		/* Set next block as old_root for b */
		b->lru_next = old_root;
		b->lru_prev = NULL;
		/* Set new root */
		cache->lru = b;
	}
	++cache->n_blocks;
	return 0;
}

//+----------------------------------------------------------------------------+
//| Pop last element from list                                                 |
//+----------------------------------------------------------------------------+

int pop_back(DB *db, block_cache *cache) {
	assert(db && cache && cache->n_blocks > 0);
	/* Find last element */
	block *last = cache->lru;
	while (last->lru_next != NULL)
		last = last->lru_next;
	/* Flush on disc if DIRTY */
	if (last->status == DIRTY)
		write_block(db, last->id, last);
	/* Change properties of previous block (if it exists) */
	if (cache->n_blocks > 1) {
		assert(last->lru_prev);
		last->lru_prev->lru_next = NULL;
	}
	/* Find block in hash table */
	hashed_pointer *hpointer;
	size_t k = last->id;
	HASH_FIND_INT(cache->hashed_blocks, &k, hpointer);
	/* Remove block from cache and hash table */
	assert(hpointer);
	HASH_DEL(cache->hashed_blocks, hpointer);
	//printf("Block %lu removed from cache, total blocks: %lu\n", last->id, cache->n_blocks - 1);
	free_block(last);
	--cache->n_blocks;
	return 0;
}