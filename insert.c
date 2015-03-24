#include "insert.h"

// TODO: insert_nonfull, split_child

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
    size_t root_id = db->info->root_index;

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
        	result = split_child(db, db->root, root_id, 0);
        	/* Check result*/
        	if (result == 0) {
        		/* Insert into nonfull branch */
        		result = insert_nonfull(db, db->root, root_id, key, value);
        	}
        } else {
        	/* No free blocks */
        	result = -1;
        }
    }
    return result;
}

//+----------------------------------------------------------------------------+
//| Insert key into nonfull branch                                             |
//+----------------------------------------------------------------------------+

int insert_nonfull(DB *db, block *b, size_t k, DBT *key, DBT *value) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(key && key->data && value && value->data);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);

    /* Result of operation */
    int result = 0;

    /* Insert key into this block or one of its children */
    if (b->num_children == 0) {
        /* Leaf block */
        result = insert_item(db, b, k, key, value);
        /* Print info *********************************************************/
        printf("\nItems (%lu):\n", b->num_keys);
        for (size_t j = 0; j < b->num_keys; ++j) {
            write(1, b->items[j]->key->data, b->items[j]->key->size);
            printf("\n");
        }
        /**********************************************************************/
    } else {
        /* Non-leaf block 
        Move from the end to the right key location
        while (i > 0 && compare_keys(key, b->items[i-1]->key))
            --i;
        size_t ch_index = b->children[i];
        block *x = db->_read_block(db, ch_index);

        Compute free memory in block and needed memory 
        size_t free_mem = free_space(x, db->info->block_size);
        size_t needed_mem = sizeof(size_t) * 2 + key->size + value->size;
        if (x->num_children > 0)
            needed_mem += 8;

        if (needed_mem > free_mem) {
            split_child(db, b, k, i);
            if (compare_keys(b->items[i]->key, key))
                ++i;
        }

        Insert key into nonfull branch 
        return insert_nonfull(db, x, ch_index, key, value);*/
        result = -1;
    }
    return result;
}

//+----------------------------------------------------------------------------+
//| Split b's child                                                            |
//+----------------------------------------------------------------------------+

int split_child(DB *db, block *x, size_t x_block, size_t child) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(child < x->num_children);

    /* Result of splitting */
    int result = 0;

    /* Find empty block */
    size_t z_block = db->_find_empty_block(db);
    result = (z_block > 0);
    /* Check if there is one more free block */
    if (result == 0) {
    	/* Remember w's location */
    	size_t w_block = x->children[child];
    	/* Read block y from disc */
    	block *w = db->_read_block(db, w_block);
    	assert(w);
    	/* Compute index of raised item from block w */
    	size_t up_me = w->num_keys / 2;
    	assert(up_me > 0 && up_me < w->num_keys - 1);
    	/* Copy left part of w to block y */
    	block *y = split_node(w, 0, up_me - 1);
    	/* Copy right part of w to block z */
    	block *z = split_node(w, up_me + 1, x->num_keys - 1);
    	/* Insert up_me item into block x */
    	item *it = w->items[up_me];
    	insert_item(db, x, x_block, it->key, it->value);
    	/* Insert new child into block x? */
	}
    return result;
}

//+----------------------------------------------------------------------------+
//| Insert item into block                                                     |
//+----------------------------------------------------------------------------+

int insert_item(DB *db, block *b, size_t k, DBT *key, DBT *value) {
	/* Check params */
	assert(b);
    assert(key && key->data && value && value->data);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);

    /* The end of key array */
	size_t i = b->num_keys;
    /* Realloc keys to array with size = size + 1 */
    b->items = (item **)realloc(b->items, (b->num_keys+1) * sizeof(item *));
    /* Create new item */
    item *it = create_item(key, value);
    /* Move from the end to the correct key location */
    while (i > 0 && compare_keys(key, b->items[i-1]->key)) {
        b->items[i] = b->items[i-1];
        --i;
    }
    /* Insert new item */
    b->items[i] = it;
    b->num_keys += 1;
    /* Write block on disc */
    return db->_write_block(db, k, b);
}

//+----------------------------------------------------------------------------+
//| Returns node with keys in range [from, to]                                 |
//+----------------------------------------------------------------------------+

block *split_node(block *x, size_t from, size_t to) {
	/* Check params */
	assert(b);
    assert(from < b->num_keys && to < b->num_keys);

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