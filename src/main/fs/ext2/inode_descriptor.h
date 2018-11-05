#ifndef MEXT2_FS_EXT2_H
#define MEXT2_FS_EXT2_H

#include <stdint.h>
#include "common.h"

struct mext2_inode_descriptor
{
    uint32_t inode_no;
    uint32_t i_size;
    uint32_t i_blocks;
    uint32_t i_dir_acl;
} __mext2_packed;

#endif
