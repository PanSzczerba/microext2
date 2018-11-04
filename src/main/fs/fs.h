#ifndef MEXT2_SD_FS_H
#define MEXT2_SD_FS_H

#include <stdint.h>
#include "ext2/ext2_descriptor.h"

struct mext2_file;
struct mext2_sd;

struct mext2_fs_descriptor
{
    uint8_t open_file_counter;
    struct mext2_file* (*open_strategy)(struct mext2_sd* sd, char* path, uint16_t mode);
    union
    {
        mext2_ext2_descriptor ext2;
    } descriptor;
};

#define MEXT2_FS_PROBE_CHAINS_SIZE 1
extern uint8_t (*mext2_fs_probe_chains[MEXT2_FS_PROBE_CHAINS_SIZE])(struct mext2_sd*);

#endif
