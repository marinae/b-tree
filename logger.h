#ifndef __LOGGER_H__
#define __LOGGER_H__

//#define _DEBUG_WAL_MODE_

#include "classes.h"
#include "blocks.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

static const int RECORD_MARKER = 0xdeadbeef;
static const int CHECKPOINT    = 0xba0babed;
static const int CP_FREQUENCY  = 100;

//+----------------------------------------------------------------------------+
//| Logger API                                                                 |
//+----------------------------------------------------------------------------+

Log    *log_open(DB *db);
void   log_close(DB *db);
int    log_write(DB *db, Record *record);
void   log_seek(DB *db);
Record *log_read_next(DB *db);

#endif /* __LOGGER_H__ */