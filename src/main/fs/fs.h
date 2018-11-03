#ifndef MEXT2_SD_FS_H
#define MEXT2_SD_FS_H
#include "ext2/ext2_descriptor.h"

enum mext2_fs_type
{
    MEXT2_INVALID_FS = -1,
    MEXT2_EXT2_FS
};

struct mext2_fs_descriptor
{
    uint64_t fs_block_addr;
    uint8_t fs_type;
    uint8_t open_file_counter;
    union
    {
        mext2_ext2_descriptor ext2;
    } descriptor;
};

#endif
