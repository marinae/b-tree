#include "delete.h"

//+----------------------------------------------------------------------------+
//| Delete key from database                                                   |
//+----------------------------------------------------------------------------+

int f_delete(DB *db, DBT *key) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(key && key->data);

    /* Delete - start from root */
    return delete_from(db, db->root, db->info->root_index, key);
}

//+----------------------------------------------------------------------------+
//| Delete key from this block or its child                                    |
//+----------------------------------------------------------------------------+

int delete_from(DB *db, block *x, size_t k, DBT *key) {
	/* Check params */
    assert(db && db->info && db->root);
    assert(x && key && key->data);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);

	/* Result of deleting */
	int result = 0;

    /* Check if block contains this key */
    size_t j = contains_key(x, key);
    if (j < x->num_keys) {
        /* Check if block x is leaf */
        if (x->num_children == 0) {
        	/* Case 1 */
        	result = delete_here(db, x, k, j);
        } else {
        	/* Case 2 */
        	/* Check if two children can be merged (and +key) */
        	if (can_merge(db, x, j)) {
        		/* Merge children and insert key into child */
        		result = merge_children(db, x, j, k, key);

        		if (result == 0) {
        			/* Read x's child (merged!) */
	    			block *subtree = db->_read_block(db, x->children[j]);
        			/* Delete key from child recursively */
        			result = delete_from(db, subtree, x->children[j], key);
        			/* Free allocated block */
	    			free_block(subtree);
        		}
        	} else {
        		/* Find biggest child of [j] and [j+1] and replace key */
        		result = replace_key(db, x, j, k, key);
        	}
        }
    } else {
    	/* Case 3 */
    	/* Check if block x is leaf */
        if (x->num_children == 0) {
        	/* Key not found in tree */

        } else {
        	/* Try to merge all children pairwise */
        	j = 0;
        	while (j+1 < x->num_children) {
	    		/* Check if two children can be merged (and +key) */
	        	if (can_merge(db, x, j)) {
	        		/* Merge children and insert key into child */
	        		merge_children(db, x, j, k, key);
	        	} else {
	        		++j;
	        	}
        	}
	    	/* Find subtree containing key */
	    	j = find_child(x, key);
	    	/* Read x's child */
	    	block *subtree = db->_read_block(db, x->children[j]);
	    	/* Delete key from this block recursively */
	    	result = delete_from(db, subtree, x->children[j], key);
	    	/* Free allocated block */
	    	result |= free_block(subtree);
    	}
    }
    return result;
}

//+----------------------------------------------------------------------------+
//| Delete key from this block and write changes                               |
//+----------------------------------------------------------------------------+

int delete_here(DB *db, block *x, size_t k, size_t j) {
	/* Check params */
    assert(db && db->info && db->root);
    assert(x && k >= db->info->first_node && k <= db->info->num_blocks);
    assert(x->num_children == 0);
    assert(j < x->num_keys);

    /* Free j-th item */
    free_item(x->items[j]);

    /* Move keys in block */
    for (int i = j; i < x->num_keys-1; ++i) {
    	x->items[i] = x->items[i+1];
    }
    x->num_keys -= 1;

    /* Write changes */
    return db->_write_block(db, k, x);
}


//+----------------------------------------------------------------------------+
//| Check if children [j] and [j+1] of x can be merged in one                  |
//+----------------------------------------------------------------------------+

bool can_merge(DB *db, block *x, size_t j) {
	/* Check params */
    assert(db && db->info && db->root);
    assert(x && j < x->num_children && (j+1) < x->num_children);

    /* Read blocks from disc */
    block *left  = db->_read_block(db, x->children[j]);
    block *right = db->_read_block(db, x->children[j+1]);
    assert(left && right);

    /* Check for errors */
    size_t left_ch  = left->num_children;
    size_t right_ch = right->num_children;
    assert(left_ch == 0 && right_ch == 0 || left_ch > 0 && right_ch > 0);

    /* Count memory needed for blocks */
    size_t left_mem  = need_memory(left);
    size_t right_mem = need_memory(right);

    /* Compute total amount of memory */
    size_t needed_mem = left_mem + right_mem;
    needed_mem += sizeof(size_t) * 2 + db->max_key_size;
    if (left->num_children > 0)
        needed_mem += 8;

    /* Free allocated memory */
    free_block(left);
    free_block(right);

    return (needed_mem <= db->info->block_size);
}

//+----------------------------------------------------------------------------+
//| Merge children [j] and [j+1] of x and delete key from x                    |
//+----------------------------------------------------------------------------+

int merge_children(DB *db, block *x, size_t j, size_t k, DBT *key) {
	/* Check params */
    assert(db && db->info && db->root);
    assert(x && key && key->data);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);
    assert(j < x->num_children && (j+1) < x->num_children);

    /* Result of merging */
    int result = 0;

    /* Read blocks from disc */
    block *left  = db->_read_block(db, x->children[j]);
    block *right = db->_read_block(db, x->children[j+1]);
    assert(left && right);
    /* Check for errors */
    size_t left_ch  = left->num_children;
    size_t right_ch = right->num_children;
    assert(left_ch == 0 && right_ch == 0 || left_ch > 0 && right_ch > 0);

    /* Append key from x */
    /* Realloc keys to array with size = size_left + size_right + 1 */
    size_t n_keys = left->num_keys + right->num_keys + 1;
    left->items = (item **)realloc(left->items, (n_keys) * sizeof(item *));
    /* Create new item for key from x */
    item *it = create_item(x->items[j]->key, x->items[j]->value);
    left->items[left->num_keys] = it;
    /* Move items from right block to left block */
    for (int i = 0; i < right->num_keys; ++i) {
    	/* Check for errors */
    	assert(left->num_keys+i+1 < n_keys);
    	/* Create new item for inserting */
    	it = create_item(right->items[i]->key, right->items[i]->value);
    	left->items[left->num_keys+i+1] = it;
    }
    left->num_keys = n_keys;

    /* Move children */
    if (right->num_children > 0) {
    	/* Reallocate memory */
    	size_t n_child = left->num_children + right->num_children;
    	left->children = (size_t *)realloc(left->children, n_child * sizeof(size_t));

	    for (int i = 0; i < right->num_children; ++i) {
	    	/* Check for errors */
	    	assert(left->num_children+i < n_child);
	    	left->children[left->num_children+i] = right->children[i];
	    }
	    left->num_children = n_child;
	}

	/* Left block is ready! */
	result = db->_write_block(db, x->children[j], left);
	/* Right block is ready! */
	result |= db->_mark_block(db, x->children[j+1], 0);

	/* Delete key from block x */
	/* Free j-th item */
    free_item(x->items[j]);
    /* Move keys in block */
    for (int i = j; i < x->num_keys-1; ++i) {
    	x->items[i] = x->items[i+1];
    }
    x->num_keys -= 1;
    /* Move children of block */
    for (int i = j+1; i < x->num_children-1; ++i) {
    	x->children[i] = x->children[i+1];
    }
    x->num_children -= 1;
    /* Write changes */
    result |= db->_write_block(db, k, x);

    /* Free allocated structures */
	free_block(left);
	free_block(right);

    return result;
}

//+----------------------------------------------------------------------------+
//| Replace key in x with key from one of its children                         |
//+----------------------------------------------------------------------------+

int replace_key(DB *db, block *x, size_t j, size_t k, DBT *key) {
	/* Check params */
    assert(db && db->info && db->root);
    assert(x && key && key->data);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);
    assert(j < x->num_children && (j+1) < x->num_children);

    /* Result of replacing key in block */
    int result = 0;

    /* Read blocks from disc */
    block *left  = db->_read_block(db, x->children[j]);
    block *right = db->_read_block(db, x->children[j+1]);

    /* Check for errors */
    size_t left_ch  = left->num_children;
    size_t right_ch = right->num_children;
    assert(left_ch == 0 && right_ch == 0 || left_ch > 0 && right_ch > 0);
    assert(left->num_keys > 0 && right->num_keys > 0);

    /* Compare child blocks */
    if (left->num_keys > right->num_keys) {
    	/* Extract predecessor of x from left subtree */
    	size_t child = x->children[j];
    	/* Go to rightmost leaf */
    	while (left->num_children > 0) {
    		/* Remember current child index */
    		child = left->children[left->num_children-1];

    		/* Change 'left block' */
     		free_block(left);
    		left = db->_read_block(db, child);

    		/* Check for errors */
    		assert(left->num_keys > 0);
    	}

    	/* Get last item of 'left' block */
    	size_t last = left->num_keys - 1;
    	item *last_item = left->items[last];

    	/* Delete item from left block */
    	item *it = create_item(last_item->key, last_item->value);
    	result = delete_here(db, left, child, last);
    	
		/* Replace key in x */
		free_item(x->items[j]);
		x->items[j] = it;
		result = db->_write_block(db, k, x);

    } else {
    	/* Extract successor of x from right subtree */
    	size_t child = x->children[j+1];
    	/* Go to leftmost leaf */
    	while (right->num_children > 0) {
    		/* Remember current child index */
    		child = right->children[0];

    		/* Change 'right block' */
     		free_block(right);
    		right = db->_read_block(db, child);

    		/* Check for errors */
    		assert(right->num_keys > 0);
    	}

    	/* Get first item of 'right' block */
    	item *first_item = right->items[0];

    	/* Delete item from right block */
    	item *it = create_item(first_item->key, first_item->value);
    	result = delete_here(db, right, child, 0);

		/* Replace key in x */
		free_item(x->items[j]);
		x->items[j] = it;
		result = db->_write_block(db, k, x);
    }

    /* Free allocated memory */
    free_block(left);
    free_block(right);

    return result;
}