#include "blocks.h"

//+----------------------------------------------------------------------------+
//| Write block                                                                |
//+----------------------------------------------------------------------------+

int write_block(struct DB *db, size_t k, struct block *b) {
    /* Check params */
    assert(db && db->info && b);
    assert(b->items || b->num_keys == 0);
    assert(b->children || b->num_children == 0);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);

    /* Calculate offset of block */
    size_t offset = db->info->block_size * k;

    /* Write key-value count */
    lseek(db->info->fd, offset, SEEK_SET);
    write(db->info->fd, (void *)&b->num_keys, sizeof(b->num_keys));

    /* Write pairs of keys and values */
    for (int i = 0; i < b->num_keys; ++i) {
        /* Assign shorter name */
        item *it = b->items[i];
        /* Check params */
        assert(it && it->key && it->key->data && it->value && it->value->data);
        /* Assign shorter names */
        DBT *key = it->key;
        DBT *val = it->value;
        /* Write key size */
        write(db->info->fd, (void *)&key->size, sizeof(key->size));
        /* Write key */
        write(db->info->fd, it->key->data, it->key->size);
        /* Write value size */
        write(db->info->fd, (void *)&val->size, sizeof(val->size));
        /* Write value */    
        write(db->info->fd, it->value->data, it->value->size);
    }

    /* Write count of child nodes */
    size_t ch_count = b->num_children;
    write(db->info->fd, (void *)&ch_count, sizeof(ch_count));

    /* Write child nodes */
    for (int j = 0; j < ch_count; ++j) {
        /* Write child index */
        size_t index = b->children[j];
        write(db->info->fd, (void *)&index, sizeof(index));
    }

    /* Mark block as busy */
    return db->_mark_block(db, k, 1);
}

//+----------------------------------------------------------------------------+
//| Read block                                                                 |
//+----------------------------------------------------------------------------+

block *read_block(DB *db, size_t k) {
    /* Check params */
    assert(db && db->info);
    assert(k >= db->info->first_node && k <= db->info->num_blocks);

    /* Calculate offset of block */
    size_t offset = db->info->block_size * k;
    /* Allocate memory for block */
    block *b = (struct block *)calloc(1, sizeof(struct block));
    /* Assign shorter name */
    int fd = db->info->fd;
    /* Read key-value count */
    lseek(fd, offset, SEEK_SET);
    read(fd, (void *)&b->num_keys, sizeof(b->num_keys));
    /* Allocate memory for items */
    b->items = (item **)calloc(b->num_keys, sizeof(item *));

    /* Read pairs of keys and values */
    for (int i = 0; i < b->num_keys; ++i) {
        /* Allocate memory for single item */
        item *it = (item *)calloc(1, sizeof(item));
        b->items[i] = it;
        /* Allocate memory for key and value */
        it->key   = (DBT *)calloc(1, sizeof(DBT));
        it->value = (DBT *)calloc(1, sizeof(DBT));
        /* Assign shorter names */
        DBT *key = it->key;
        DBT *val = it->value;
        /* Read key size */
        read(fd, (void *)&key->size, sizeof(key->size));
        key->data = (void *)calloc(1, key->size);
        /* Read key */
        read(fd, key->data, key->size);
        /* Read value size */
        read(fd, (void *)&val->size, sizeof(val->size));
        val->data = (void *)calloc(1, val->size);
        /* Read value */    
        read(fd, val->data, val->size);
    }

    /* Read count of child nodes */
    read(fd, (void *)&b->num_children, sizeof(b->num_children));
    /* Check params */
    if (b->num_children != b->num_keys + 1 && b->num_children != 0) {
        /* Free all allocated memory */
        for (int j = 0; j < b->num_keys; ++j) {
            free(b->items[j]->key->data);
            free(b->items[j]->value->data);
            free(b->items[j]->key);
            free(b->items[j]->value);
            free(b->items[j]);
        }
        free(b->items);
        free(b);
        return NULL;
    }
    b->children = (size_t *)calloc(b->num_children, sizeof(size_t));

    /* Read child nodes */
    for (int j = 0; j < b->num_children; ++j) {
        /* Read child index */
        read(fd, (void *)&b->children[j], sizeof(b->children[j]));
    }
    return b;
}

//+----------------------------------------------------------------------------+
//| Find empty block                                                           |
//+----------------------------------------------------------------------------+

size_t find_empty_block(struct DB *db) {
    /* Check params */
    if (!db || !db->info || !db->info->bitmap)
        return 0;

    /* Iterate through all byte-blocks */
    for (int i = 0; i < db->info->bitmap_len; ++i) {
        /* Check if there are any free bits in this block */
        if (db->info->bitmap[i] != 0xffffffff) {
            /* Set name */
            char bit = db->info->bitmap[i];
            /* Iterate through all bits in byte */
            for (int j = 0; j < 8; ++j) {
                /* Check if bit j is free */
                char j_bit = 1 << (7 - j);

                if ((bit | j_bit) != bit)
                    return db->info->first_node + i * 8 + j;
            }
            return 0;
        }
    }
    return 0;
}

//+----------------------------------------------------------------------------+
//| Mark block as free (busy)                                                  |
//+----------------------------------------------------------------------------+

int mark_block(struct DB *db, size_t k, bool state) {
    /* Check params */
    if (!db || !db->info || !db->info->bitmap)
        return -1;

    /* Assign shorter name */
    DB_info *info = db->info;
    /* Check bounds */
    if (k > info->num_blocks || k < info->first_node)
        return -1;
    /* Subtract header blocks */
    k -= info->first_node;
    /* Compute indices */
    int i = k / 8;
    int j = k - i * 8;
    /* Set j-th bit to 1 */
    char new_bit = 1 << (7 - j);
    /* Set actual bit state */
    if (state) {
        /* Mark as busy */
        info->bitmap[i] |= new_bit;
    } else {
        /* Mark as free */
        new_bit ^= 255;
        info->bitmap[i] &= new_bit;
    }
    /* Sync with disc */
    lseek(info->fd, info->block_size, SEEK_SET);
    write(info->fd, (void *)info->bitmap, info->bitmap_len);

    return 0;
}