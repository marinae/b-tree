#include "select.h"

//+----------------------------------------------------------------------------+
//| Select key                                                                 |
//+----------------------------------------------------------------------------+

int f_select(DB *db, DBT *key, DBT *value) {
    /* Check params */
    assert(db && db->info && db->root);
    assert(key && key->data && value);

    return key_search(db, db->root, key, value);
}

//+----------------------------------------------------------------------------+
//| Search key in subtree                                                      |
//+----------------------------------------------------------------------------+

int key_search(DB *db, block *cur, DBT *key, DBT *value) {
	/* Check params */
    assert(db && db->info && db->root);
    assert(cur && key && key->data && value);

    /* Result of selection */
    int result = 0;
    /* Current key index */
    size_t i = 0;
    /* Increase key index while key[i] < key */
    while (i < cur->num_keys && (compare_keys(cur->items[i]->key, key) < 0))
    	++i;
    /* Check key */
    if (i < cur->num_keys && (compare_keys(cur->items[i]->key, key) == 0)) {
    	/* Key found */
    	result = alloc_value(cur->items[i], value);

    } else if (cur->num_children == 0) {
    	/* Leaf node, key not found */
    	/*printf("key not found in leaf: %s\n", key->data);
    	printf("Items in block:\n");
        for (int j = 0; j < cur->num_keys; ++j)
            printf("%s\n", cur->items[j]->key->data);
        printf("*************************\n");*/
    	result = -1;

    } else {
    	/* Key is in child node */
    	block *child = db->_read_block(db, cur->children[i]);
    	assert(child);
    	result = key_search(db, child, key, value);
    	free_block(child);
    }
    return result;
}

//+----------------------------------------------------------------------------+
//| Key found - alloc value->data and fill it                                  |
//+----------------------------------------------------------------------------+

int alloc_value(item *it, DBT *value) {
	/* Check params */
    assert(it && it->key && it->value && value);

    value->size = it->value->size;
    value->data = (void *)calloc(1, value->size);
    memcpy(value->data, it->value->data, value->size);

    return 0;
}