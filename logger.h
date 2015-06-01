#ifndef __LOGGER_H__
#define __LOGGER_H__

//#define _DEBUG_WAL_MODE_

#include "classes.h"
#include "blocks.h"
#include "cache.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

static const char WAL_FILE[]    = "wal";
static const int  RECORD_MARKER = 0xdeadbeef;
static const int  CHECKPOINT    = 0xba0babed;
static const int  CP_FREQUENCY  = 100;

//+----------------------------------------------------------------------------+
//| WAL format:                                                                |
//|   - RECORD_MARKER (int)                                                    |
//|   - block_id      (size_t)                                                 |
//| - block:                                                                   |
//|   - lsn           (size_t)                                                 |
//|   - num_keys      (size_t)                                                 |
//|   - ...                                                                    |
//|   - key_size      (size_t)                                                 |
//|   - key_data      (key_size)                                               |
//|   - value_size    (size_t)                                                 |
//|   - value_data    (value_size)                                             |
//|   - num_children  (size_t)                                                 |
//|  (- ...                   )                                                |
//|  (- child_id      (size_t))                                                |
//|  (- ...                   )                                                |
//|   - ...                                                                    |
//| - checkpoint: (every CP_FREQUENCY records in WAL)                          |
//|   - CHECKPOINT    (int)                                                    |
//|   - lsn           (size_t)                                                 |
//+----------------------------------------------------------------------------+

//+----------------------------------------------------------------------------+
//| Logger API                                                                 |
//+----------------------------------------------------------------------------+

Log    *log_open(DB *db, char *log_file_name);
void   log_close(DB *db);
int    log_write(DB *db, Record *record);
void   log_seek(DB *db);
Record *log_read_next(DB *db);

#endif /* __LOGGER_H__ */