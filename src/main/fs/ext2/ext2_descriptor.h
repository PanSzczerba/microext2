#ifndef MEXT2_FS_EXT2_EXT2_DESCRIPTOR_H
#define MEXT2_FS_EXT2_EXT2_DESCRIPTOR_H

#include <stdint.h>
#include "common.h"

struct mext2_sd;
struct mext2_ext2_superblock;

struct mext2_ext2_superblock_shortcut
{
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_blocks_per_group;
    uint32_t s_inodes_per_group;
/*** EXT2_DYNAMIC_REV Specific ***/
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
} __mext2_packed;

typedef struct mext2_ext2_superblock_shortcut mext2_ext2_descriptor;
#endif
