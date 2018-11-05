#ifndef MEXT2_FS_EXT2_EXT2_DESCRIPTOR_H
#define MEXT2_FS_EXT2_EXT2_DESCRIPTOR_H

#include <stdint.h>
#include "common.h"

#define EXT2_SUPERBLOCK_PARTITION_BLOCK_OFFSET 2

struct mext2_sd;
struct mext2_ext2_superblock;

struct mext2_ext2_superblock_shortcut
{
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_log_block_size;
    uint32_t s_blocks_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
} __mext2_packed;

typedef struct mext2_ext2_superblock_shortcut mext2_ext2_descriptor;

void mext2_ext2_sd_filler(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock);

#endif