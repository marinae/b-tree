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

    size_t i = b->num_keys;

    /* Insert key into this block or one of its children */
    if (b->num_children == 0) {
        /* Leaf block */
        /* Realloc keys to array with size = size + 1 */
        b->items = (item **)realloc(b->items, (b->num_keys+1) * sizeof(item *));

        /* Create new item */
        item *it = create_item(key, value);

        /* Move from the end to the right key location */
        while (i > 0 && compare_keys(key, b->items[i-1]->key)) {
            b->items[i] = b->items[i-1];
            --i;
        }

        /* Insert new item */
        b->items[i] = it;
        b->num_keys += 1;

        printf("\nItems (%lu):\n", b->num_keys);
        for (size_t j = 0; j < b->num_keys; ++j) {
            //printf("%s\n", b->items[j]->key->data);
            write(1, b->items[j]->key->data, b->items[j]->key->size);
            printf("\n");
        }

        /* Write block on disc */
        return db->_write_block(db, k, b);

    } else {
        /* Non-leaf block */
        /* Move from the end to the right key location */
        while (i > 0 && compare_keys(key, b->items[i-1]->key))
            --i;
        size_t ch_index = b->children[i];
        block *x = db->_read_block(db, ch_index);

        /* Compute free memory in block and needed memory */
        size_t free_mem = free_space(x, db->info->block_size);
        size_t needed_mem = sizeof(size_t) * 2 + key->size + value->size;
        if (x->num_children > 0)
            needed_mem += 8;

        if (needed_mem > free_mem) {
            split_child(db, b, k, i);
            if (compare_keys(b->items[i]->key, key))
                ++i;
        }

        /* Insert key into nonfull branch */
        return insert_nonfull(db, x, ch_index, key, value);
    }
}

//+----------------------------------------------------------------------------+
//| Split b's child[index]                                                     |
//+----------------------------------------------------------------------------+

int split_child(DB *db, block *b, size_t b_block, size_t index) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(index < b->num_children);

    /* Find empty block */
    size_t z_block = db->_find_empty_block(db);
    if (z_block == 0)
        return -1;
    size_t y_block = b->children[index];

    /* Allocate memory for new blocks */
    block *z = (block *)calloc(1, sizeof(block));
    block *y = db->_read_block(db, y_block);

    /* Copy rightmost part of b's child (y) to z */
    // TODO: Handle situation when it's not enough
    z->num_keys = y->num_keys / 2;
    z->items = (item **)calloc(z->num_keys, sizeof(item *));
    for (size_t i = z->num_keys; i > 0; --i) {
        size_t cur = y->num_keys - z->num_keys + i - 1;
        assert(cur < y->num_keys);
        z->items[i - 1] = create_item(y->items[cur]->key, y->items[cur]->value);
    }

    /* Copy rightmost part of y's children to z */
    if (y->num_children > 0) {
        /* Non-leaf node */
        assert(y->num_children == y->num_keys + 1);
        z->num_children = z->num_keys + 1;
        z->children = (size_t *)calloc(z->num_children, sizeof(size_t));
        for (size_t j = z->num_children; j > 0; --j) {
            size_t cur = y->num_children - z->num_children + j - 1;
            assert(cur < y->num_children);
            z->children[j - 1] = y->children[cur];
        }
    }

    /* Index of key which will be raised */
    size_t up_me = y->num_keys - z->num_keys - 1;
    assert(up_me > 0 && up_me < y->num_keys);

    /* Copy middle item */
    item *raise_me = create_item(y->items[up_me]->key, y->items[up_me]->value);

    /* Free copied items */
    for (size_t i = up_me; i < y->num_keys; ++i) {
        free_item(y->items[i]);
    }
    y->items = (item **)realloc(y->items, up_me * sizeof(item *));
    y->num_keys = up_me;

    /* Free copied children */
    for (size_t j = up_me + 1; j < y->num_children; ++j) {
        free_item(y->items[j]);
    }
    y->children = (size_t *)realloc(y->children, (up_me + 1) * sizeof(size_t));
    y->num_children = up_me + 1;

    printf("\nItems after split (block y):\n");
    for (size_t j = 0; j < y->num_keys; ++j) {
        write(1, y->items[j]->key->data, y->items[j]->key->size);
        printf("\n");
    }

    printf("\nItems after split (block z):\n");
    for (size_t j = 0; j < z->num_keys; ++j) {
        write(1, z->items[j]->key->data, z->items[j]->key->size);
        printf("\n");
    }

    /* Write block y to disc */
    // TODO: Handle errors?
    db->_write_block(db, y_block, y);
    free_block(y);

    /* Write block z to disc */
    // TODO: Handle errors?
    db->_write_block(db, z_block, z);
    free_block(z);

    /* Insert raised item into block b */
    if (b->num_keys > 0)
        b->items = (item **)realloc(b->items, (b->num_keys + 1) * sizeof(item *));
    else
        b->items = (item **)calloc(1, sizeof(item *));
    b->num_keys += 1;
    for (int i = b->num_keys - 1; i > index; --i) {
        b->items[i] = create_item(b->items[i-1]->key, b->items[i-1]->value);
        free_item(b->items[i-1]);
    }
    b->items[index] = create_item(raise_me->key, raise_me->value);
    free_item(raise_me);

    /* Correct child nodes */
    b->num_children += 1;
    size_t ch_count = b->num_children;
    b->children = (size_t *)realloc(b->children, ch_count * sizeof(size_t));
    for (int j = ch_count - 1; j > index + 1; --j) {
        b->children[j] = b->children[j - 1];
    }
    b->children[index + 1] = z_block;

    /* Write block b to disc */
    // TODO: Handle errors?
    db->_write_block(db, b_block, b);

    printf("\nItems after split (block b):\n");
    for (size_t j = 0; j < b->num_keys; ++j) {
        write(1, b->items[j]->key->data, b->items[j]->key->size);
        printf("\n");
    }

    return 0;
}