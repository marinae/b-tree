#include "cache.h"

//+----------------------------------------------------------------------------+
//| Write block (+cache)                                                       |
//+----------------------------------------------------------------------------+

int c_write_block(DB *db, size_t k, block *b_new) {
	assert(db && b_new);
	/* Fill LSN field in block */
	b_new->lsn = db->logger->log_count;
	/* Create WAL record */
	Record record = {
		.block_id = k,
		.b        = b_new
	};
	/* Write changed page to WAL */
	if (log_write(db, &record))
		return 1;
	/* Search this block in cache */
	block *b = search_in_cache(db->cache->hashed_blocks, k);
	if (b != NULL) {
		/* Found -> update block and status */
		if (b->status == CLEAN) {
			b->status = DIRTY;
			#ifdef _DEBUG_CACHE_MODE_
			printf("Block %lu marked as DIRTY\n", k);
			#endif /* _DEBUG_CACHE_MODE_ */
		}
		return copy_block_to(b, b_new);

	} else {
		return write_block(db->info->fd, db, k, b_new);
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
		#ifdef _DEBUG_CACHE_MODE_
		if (b->status == CLEAN)
			printf("Block %lu found in cache, status = CLEAN\n", k);
		else
			printf("Block %lu found in cache, status = DIRTY\n", k);
		#endif /* _DEBUG_CACHE_MODE_ */
		repush_front(db->cache, b);
		return copy_block(b);

	} else {
		/* Not found -> read from disc and add to cache */
		printf("Reading block k = %lu (c read block)\n", k);
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
			printf("Reading block k = %lu (add_to cache 1)\n", k);
			return read_block(db->info->fd, db, k);
		} else {
			pop_back(db, db->cache);
		}
	}
	assert(db->cache->n_blocks < db->cache->max_blocks);
	/* Read block from disc */
	printf("Reading block k = %lu (add_to cache 2)\n", k);
	block *b = read_block(db->info->fd, db, k);
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
	#ifdef _DEBUG_CACHE_MODE_
	printf("Block %lu added to cache, free space = %lu\n", k, db->cache->max_blocks - db->cache->n_blocks);
	#endif /* _DEBUG_CACHE_MODE_ */
	return b;
}

//+----------------------------------------------------------------------------+
//| Prepend block to double-linked list                                        |
//+----------------------------------------------------------------------------+

int push_front(block_cache *cache, block *b) {
	assert(cache && b && cache->n_blocks < cache->max_blocks);
	/* Increase number of blocks in cache */
	if (cache->n_blocks == 0) {
		/* No blocks in LRU */
		cache->lru     = b;
		cache->lru_end = b;
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
//| Move block to the first position in list                                   |
//+----------------------------------------------------------------------------+

int repush_front(block_cache *cache, block *b) {
	assert(cache && b);
	block *prev = b->lru_prev;
	block *next = b->lru_next;
	/* Check if block is already first */
	if (prev != NULL) {
		/* Update previous block */
		prev->lru_next = next;
		if (next == NULL) {
			/* Change end pointer */
			cache->lru_end = prev;
		} else {
			/* Else update next element */
			next->lru_prev = prev;
		}
		--cache->n_blocks;
		push_front(cache, b);
	}
	return 0;
}

//+----------------------------------------------------------------------------+
//| Pop last element from list                                                 |
//+----------------------------------------------------------------------------+

int pop_back(DB *db, block_cache *cache) {
	assert(db && cache && cache->n_blocks > 0);
	/* Find last element */
	block *last = cache->lru_end;
	/* Flush on disc if DIRTY */
	if (last->status == DIRTY) {
		if (write_block(db->info->fd, db, last->id, last))
			return 1;
	}
	/* Change properties of previous block (if it exists) */
	if (cache->n_blocks > 1) {
		assert(last->lru_prev);
		last->lru_prev->lru_next = NULL;
		cache->lru_end = last->lru_prev;
	}
	/* Find block in hash table */
	hashed_pointer *hpointer;
	size_t k = last->id;
	HASH_FIND_INT(cache->hashed_blocks, &k, hpointer);
	/* Remove block from cache and hash table */
	assert(hpointer);
	HASH_DEL(cache->hashed_blocks, hpointer);
	#ifdef _DEBUG_CACHE_MODE_
	printf("Block %lu removed from cache, total blocks: %lu\n", last->id, cache->n_blocks - 1);
	#endif /* _DEBUG_CACHE_MODE_ */
	free_block(last);
	--cache->n_blocks;
	return 0;
}

//+----------------------------------------------------------------------------+
//| Flush cache and clear                                                      |
//+----------------------------------------------------------------------------+

int flush_cache(DB *db, size_t lsn) {
	assert(db && db->cache);
	block *b = db->cache->lru;
	while (b) {
		if (b->status == DIRTY) {
			printf("Flushing block %lu, lsn = %lu\n", b->id, lsn);
			if (db->_write_block(db, b->id, b))
				return 1;
		}
		/*#ifdef _DEBUG_CACHE_MODE_
		printf("Block %lu removed from cache, total blocks: %lu\n", b->id, db->cache->n_blocks - 1);
		#endif * _DEBUG_CACHE_MODE_ *
		block *tmp = b;*/
		b = b->lru_next;
		/*free_block(tmp);
		--db->cache->n_blocks;*/
	}
	//HASH_CLEAR(hh, db->cache->hashed_blocks);
	return 0;
}