#include "insert.h"

//+----------------------------------------------------------------------------+
//| Insert key                                                                 |
//+----------------------------------------------------------------------------+

int f_insert(DB *db, DBT *key, DBT *value) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(key && key->data && value && value->data);

    /* Result of inserting */
    int result = 0;

    /* Assign shorter name */
    size_t root_id = db->info->hdr->root_index;

    /* Compare free memory with busy */
    if (enough_mem(db, db->root, key, value)) {
    	/* Insert into nonfull branch */
    	result = insert_nonfull(db, db->root, root_id, key, value);
    } else {
    	/* Root is full -> split root */
    	/* Try to find empty block in file */
    	size_t empty = db->_find_empty_block(db);
        if (empty > 0) {
        	/* Make this block new root */
        	make_root(db, empty);
        	/* Split child (old root) */
        	result = split_child(db, db->root, empty, 0);
        	/* Check result */
        	if (result == 0) {
        		/* Insert into nonfull branch */
        		assert(enough_mem(db, db->root, key, value));
        		result = insert_nonfull(db, db->root, empty, key, value);
        	}
        } else {
        	/* No free blocks */
        	result = -1;
            printf("No more free blocks\n");
        }
    }
    /* Report error */
    if (result != 0)
        printf("Error while inserting key %s\n", key->data);

    /* Update current maximum key+value size in database */
    if (key->size + value->size > db->info->hdr->max_key_size) {
        db->info->hdr->max_key_size = key->size + value->size;

        /* Write changes to file */
        lseek(db->info->fd, 0, SEEK_SET);
        write(db->info->fd, (void*)db->info->hdr, sizeof(*(db->info->hdr)));
    }

    return result;
}

//+----------------------------------------------------------------------------+
//| Insert key into nonfull branch                                             |
//+----------------------------------------------------------------------------+

int insert_nonfull(DB *db, block *x, size_t k, DBT *key, DBT *value) {
    /* Check params */
    assert(db && db->info && db->root && x);
    assert(key && key->data && value && value->data);
    assert(k >= db->info->hdr->first_node && k <= db->info->hdr->num_blocks);

    /* Result of operation */
    int result = 0;

    /* Insert key into this block or one of its children */
    if (x->num_children == 0) {
        /* Leaf block */
        assert(enough_mem(db, x, key, value));
        result = insert_item(db, x, k, key, value, 0);

    } else {
        /* Non-leaf block */
        /* Check if element already exists here */
        size_t j = contains_key(x, key);
        if (j < x->num_keys) {
            /* Edit value for key */
            return replace_value(db, k, x, j, key, value);
        }
        /* Find child containing specific key range */
        int i = find_child(x, key);
        /* Read child block */
        block *y = db->_read_block(db, x->children[i]);
        /* Check if block is full */
        if (!enough_mem(db, y, key, value)) {
        	/* Block is full */
        	result = split_child(db, x, k, i);
        	free_block(y);
            /* Check if new value splitted into x equals key */
            j = contains_key(x, key);
            if (j < x->num_keys) {
                /* Edit value for key */
                return replace_value(db, k, x, j, key, value);
            }
        	y = db->_read_block(db, x->children[i]);
        	/* Compare key with new child */
        	if (compare_keys(x->items[i]->key, key) < 0) {
        		/* Load new child block */
        		++i;
        		free_block(y);
        		y = db->_read_block(db, x->children[i]);
        	}
        }  
        /* Insert into nonfull branch */
        assert(enough_mem(db, y, key, value));
        result |= insert_nonfull(db, y, x->children[i], key, value);
    }
    return result;
}

//+----------------------------------------------------------------------------+
//| Split b's child                                                            |
//+----------------------------------------------------------------------------+

int split_child(DB *db, block *x, size_t x_block, size_t child) {
    /* Check params */
    assert(db && db->info && db->root && x);
    assert(child < x->num_children);

    /* Result of splitting */
    int result = 0;

    /* Find empty block */
    size_t z_block = db->_find_empty_block(db);
    result = (z_block > 0) ? 0 : -1;
    /* Check if there is one more free block */
    if (result == 0) {
    	/* Remember w's location */
    	size_t w_block = x->children[child];
    	assert(x_block != w_block && x_block != z_block && w_block != z_block);
    	/* Read block y from disc */
    	block *w = db->_read_block(db, w_block);
    	assert(w);
    	/* Compute index of raised item from block w */
    	size_t up_me = w->num_keys / 2;
    	assert(up_me > 0 && up_me < w->num_keys - 1);
    	/* Copy left part of w to block y */
    	block *y = split_node(w, 0, up_me - 1);
    	/* Copy right part of w to block z */
    	block *z = split_node(w, up_me + 1, w->num_keys - 1);
    	/* Insert up_me item into block x */
    	item *it = w->items[up_me];
    	if (!enough_mem(db, x, it->key, it->value)) {
    		/* Free allocated structures */
    		free_block(y);
    		free_block(z);
    		printf("Error while inserting key (in split_node)\n");
    		return -1;
    	}
    	result = insert_item(db, x, x_block, it->key, it->value, z_block);
    	/* Write blocks y and z to disc */
    	assert(need_memory(y) <= db->info->hdr->block_size);
    	assert(need_memory(z) <= db->info->hdr->block_size);
    	result |= db->_write_block(db, w_block, y);
    	result |= db->_write_block(db, z_block, z);
    	/* Free allocated structures */
    	free_block(y);
    	free_block(z);
	}
    return result;
}

//+----------------------------------------------------------------------------+
//| Insert item into block                                                     |
//+----------------------------------------------------------------------------+

int insert_item(DB *db, block *x, size_t k, DBT *key, DBT *value, size_t chd) {
	/* Check params */
	assert(db && db->info && x);
    assert(key && key->data && value && value->data);
    assert(k >= db->info->hdr->first_node && k <= db->info->hdr->num_blocks);

    /* Index of inserting */
    size_t i = contains_key(x, key);
    /* Check if element already exists */
    if (i < x->num_keys) {
        return replace_value(db, k, x, i, key, value);
    }

    /* The end of key array */
	i = x->num_keys;
    /* Realloc keys to array with size = size + 1 */
    x->items = (item **)realloc(x->items, (x->num_keys+1) * sizeof(item *));
    /* Create new item */
    item *it = create_item(key, value);
    /* Move from the end to the correct key location */
    while (i > 0 && (compare_keys(key, x->items[i-1]->key) < 0)) {
        x->items[i] = x->items[i-1];
        --i;
    }
    /* Insert new item */
    x->items[i] = it;
    x->num_keys += 1;
    /* Insert child if it is needed (chd > 0) */
    if (chd > 0) {
    	/* Check node state */
    	size_t n = x->num_children;
    	assert(n > 0);
    	/* Add child to position (i + 1) */
    	x->children = (size_t *)realloc(x->children, (n + 1) * sizeof(size_t));
    	x->num_children += 1;
    	for (int j = x->num_children - 1; j > i + 1; --j)
    		x->children[j] = x->children[j - 1];
    	x->children[i + 1] = chd;
    }
    /* Write block on disc */
    assert(need_memory(x) <= db->info->hdr->block_size);
    return db->_write_block(db, k, x);
}

//+----------------------------------------------------------------------------+
//| Returns node with keys in range [from, to]                                 |
//+----------------------------------------------------------------------------+

block *split_node(block *x, size_t from, size_t to) {
	/* Check params */
	assert(x);
    assert(from < x->num_keys && to < x->num_keys);

    /* Allocate memory for new block */
    block *y = (block *)calloc(1, sizeof(block));
    /* Fill block */
    y->num_keys = to - from + 1;
    y->items = (item **)calloc(y->num_keys, sizeof(item *));
    for (int i = 0; i < y->num_keys; ++i) {
    	/* Assign shorter name */
    	item *it = x->items[from + i];
    	/* Copy item */
    	y->items[i] = create_item(it->key, it->value);
    }
    /* Fill children */
    if (x->num_children > 0) {
    	y->num_children = y->num_keys + 1;
    	y->children = (size_t *)calloc(y->num_children, sizeof(size_t));
    	for (int j = 0; j < y->num_children; ++j) {
    		/* Copy child */
    		y->children[j] = x->children[from + j];
    	}
    }
    return y;
}

//+----------------------------------------------------------------------------+
//| Replace old value                                                          |
//+----------------------------------------------------------------------------+

int replace_value(DB *db, size_t k, block *x, size_t j, DBT *key, DBT *val) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(x && key && key->data && val && val->data);

    x->items[j]->value->size = val->size;
    memcpy(x->items[j]->value->data, val->data, val->size);

    return db->_write_block(db, k, x);
}