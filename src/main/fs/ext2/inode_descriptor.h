#ifndef MEXT2_FS_EXT2_INODE_DESCRIPTOR_H
#define MEXT2_FS_EXT2_INODE_DESCRIPTOR_H

#include <stdint.h>
#include "common.h"
#include "inode.h"

struct mext2_inode_descriptor
{
    uint32_t inode_no;
    struct mext2_ext2_inode_address inode_address;
    uint32_t i_blocks;
    uint64_t i_size;
} __mext2_packed;
#endif
