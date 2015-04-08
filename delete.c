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
        	result = delete_here(db, x, k, key, j);

        } else {
        	/* Case 2 */
        }
    } else {
    	/* Case 3 */
    }

    return 0;
}

//+----------------------------------------------------------------------------+
//| Delete key from this block and write changes                               |
//+----------------------------------------------------------------------------+

int delete_here(DB *db, block *x, size_t k, DBT *key, size_t j) {
	/* Check params */
    assert(db && db->info && db->root);
    assert(x && key && key->data);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);
    assert(x->num_children == 0);
    assert(j < x->num_keys);

    /* Free j-th item in block */
    free_item(x->items[j]);
    x->num_keys -= 1;

    /* Move keys in block */
    for (int i = j+1; i < x->num_keys; ++i)
    	x->items[i-1] = x->items[i];

    printf("Key deleted from leaf: %s\n", key->data);

    /* Write changes */
    return db->_write_block(db, k, x);
}