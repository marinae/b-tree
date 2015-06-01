#include "blocks.h"

//+----------------------------------------------------------------------------+
//| Write block                                                                |
//+----------------------------------------------------------------------------+

int write_block(int fd, struct DB *db, size_t k, struct block *b) {
    /* Check params */
    assert(db && db->info && b);
    assert(b->items || b->num_keys == 0);
    assert(b->children || b->num_children == 0);
    assert(k >= db->info->hdr->first_node && k <= db->info->hdr->num_blocks);
    assert(need_memory(b) <= db->info->hdr->block_size);

    /* Calculate offset of block */
    size_t offset = db->info->hdr->block_size * k;

    /* Change offset */
    if (fd == db->info->fd)
        lseek(fd, offset, SEEK_SET);

    /* Write LSN */
    if (-1 == write(fd, (void *)&b->lsn, sizeof(b->lsn)))
        return 1;

    /* Write key-value count */
    if (-1 == write(fd, (void *)&b->num_keys, sizeof(b->num_keys)))
        return 1;

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
        if (-1 == write(fd, (void *)&key->size, sizeof(key->size)))
            return 1;
        /* Write key */
        if (-1 == write(fd, it->key->data, it->key->size))
            return 1;
        /* Write value size */
        if (-1 == write(fd, (void *)&val->size, sizeof(val->size)))
            return 1;
        /* Write value */    
        if (-1 == write(fd, it->value->data, it->value->size))
            return 1;
    }

    /* Write count of child nodes */
    size_t ch_count = b->num_children;
    if (-1 == write(fd, (void *)&ch_count, sizeof(ch_count)))
        return 1;

    /* Write child nodes */
    for (int j = 0; j < ch_count; ++j) {
        /* Write child index */
        size_t index = b->children[j];
        if (-1 == write(fd, (void *)&index, sizeof(index)))
            return 1;
    }

    /* Mark block as busy */
    if (fd == db->info->fd)
        return db->_mark_block(db, k, 1);
    return 0;
}

//+----------------------------------------------------------------------------+
//| Read block                                                                 |
//+----------------------------------------------------------------------------+

block *read_block(int fd, DB *db, size_t k) {
    /* Check params */
    assert(db && db->info);
    assert(k >= db->info->hdr->first_node && k <= db->info->hdr->num_blocks);

    /* Calculate offset of block */
    size_t offset = db->info->hdr->block_size * k;

    /* Allocate memory for block */
    block *b = (struct block *)calloc(1, sizeof(struct block));

    /* Change offset */
    if (fd == db->info->fd)
        lseek(fd, offset, SEEK_SET);

    /* Read LSN */
    read(fd, (void *)&b->lsn, sizeof(size_t));
    /* Read key-value count */
    read(fd, (void *)&b->num_keys, sizeof(size_t));
    /* Allocate memory for items */
    b->items = (item **)calloc(b->num_keys, sizeof(item *));
    assert(b->items);
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
        printf("An error occured in blocks.c (read): block %lu\n", k);
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
    if (!db || !db->info || !db->info->bitmap) {
        printf("An error occured in blocks.c (find)\n");
        return 0;
    }

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
                    return db->info->hdr->first_node + i * 8 + j;
            }
            printf("An error occured in blocks.c (find)\n");
            return 0;
        }
    }
    printf("An error occured in blocks.c (find)\n");
    return 0;
}

//+----------------------------------------------------------------------------+
//| Mark block as free (busy)                                                  |
//+----------------------------------------------------------------------------+

int mark_block(struct DB *db, size_t k, bool state) {
    /* Check params */
    if (!db || !db->info || !db->info->bitmap) {
        printf("An error occured in blocks.c (mark)\n");
        return -1;
    }

    /* Assign shorter name */
    DB_info *info = db->info;
    /* Check bounds */
    if (k > info->hdr->num_blocks || k < info->hdr->first_node) {
        printf("An error occured in blocks.c (mark)\n");
        return -1;
    }
    /* Subtract header blocks */
    k -= info->hdr->first_node;
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
    lseek(info->fd, info->hdr->bitmap_offset, SEEK_SET);
    if (-1 == write(info->fd, (void *)info->bitmap, info->bitmap_len))
        return 1;

    return 0;
}

//+----------------------------------------------------------------------------+
//| Return copy of block                                                       |
//+----------------------------------------------------------------------------+

block *copy_block(block *b) {
    assert(b);
    block *b_copy = (block *)calloc(1, sizeof(block));
    blockcpy(b, b_copy);
    return b_copy;
}

//+----------------------------------------------------------------------------+
//| Replace b's items and children with b_new's                                |
//+----------------------------------------------------------------------------+

int copy_block_to(block *b, block *b_new) {
    assert(b && b_new);
    /* Free all internal structures of b */
    if (b->items) {
        for (size_t i = 0; i < b->num_keys; ++i) {
            if (b->items[i]) {
                free(b->items[i]);
                b->items[i] = NULL;
            }
        }
        free(b->items);
        b->items = NULL;
    }
    if (b->children) {
        free(b->children);
        b->children = NULL;
    }
    blockcpy(b_new, b);
    return 0;
}

//+----------------------------------------------------------------------------+
//| Allocate memory and copy items/children (from b to b_copy)                 |
//+----------------------------------------------------------------------------+

int blockcpy(block *b, block *b_copy) {
    assert(b && b_copy);
    /* Copy items */
    b_copy->num_keys = b->num_keys;
    b_copy->items = (item **)calloc(b->num_keys, sizeof(item *));
    for (int i = 0; i < b->num_keys; ++i) {
        b_copy->items[i] = (item *)calloc(1, sizeof(item));
        item *it  = b->items[i];
        item *itc = b_copy->items[i];
        /* Copy key */
        itc->key = (DBT *)calloc(1, sizeof(DBT));
        itc->key->size = it->key->size;
        itc->key->data = (void *)calloc(1, it->key->size);
        memcpy(itc->key->data, it->key->data, it->key->size);
        /* Copy value */
        itc->value = (DBT *)calloc(1, sizeof(DBT));
        itc->value->size = it->value->size;
        itc->value->data = (void *)calloc(1, it->value->size);
        memcpy(itc->value->data, it->value->data, it->value->size);
    }
    /* Copy children */
    b_copy->num_children = b->num_children;
    if (b->num_children > 0) {
        b_copy->children = (size_t *)calloc(b->num_children, sizeof(size_t));
        for (int j = 0; j < b->num_children; ++j)
            b_copy->children[j] = b->children[j];
    }
    return 0;
}

//+----------------------------------------------------------------------------+
//| Read only LSN of a block k                                                 |
//+----------------------------------------------------------------------------+

size_t get_lsn(DB *db, size_t k) {
    /* Check params */
    assert(db && db->info);
    assert(k >= db->info->hdr->first_node && k <= db->info->hdr->num_blocks);

    /* Calculate offset of block */
    size_t offset = db->info->hdr->block_size * k;

    /* Change offset */
    lseek(db->info->fd, offset, SEEK_SET);

    /* Read LSN */
    size_t lsn;
    read(db->info->fd, (void *)&lsn, sizeof(size_t));

    return lsn;
}