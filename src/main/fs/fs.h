#ifndef MEXT2_SD_FS_H
#define MEXT2_SD_FS_H

#include <stdint.h>
#include "ext2/ext2_descriptor.h"
#include "file.h"

struct mext2_file;
struct mext2_sd;

struct mext2_fs_descriptor
{
    mext2_open_strategy open_strategy;
    mext2_read_strategy write;
    mext2_write_strategy read;
    mext2_seek_strategy seek;
    mext2_close_strategy close;
    mext2_eof_strategy eof;
    uint8_t open_file_counter;
    union
    {
        mext2_ext2_descriptor ext2;
    } descriptor;
};

#define MEXT2_FS_PROBE_CHAINS_SIZE 1
typedef uint8_t (*mext2_probe_fs_strategy)(struct mext2_sd*);

extern mext2_probe_fs_strategy mext2_fs_probe_chains[MEXT2_FS_PROBE_CHAINS_SIZE];

#endif
