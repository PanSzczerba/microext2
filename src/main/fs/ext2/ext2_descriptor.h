#ifndef MEXT2_FS_EXT2_EXT2_DESCRIPTOR_H
#define MEXT2_FS_EXT2_EXT2_DESCRIPTOR_H

struct mext2_ext2_superblock_shortcut
{
    uint64_t superblock_pos;
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_log_block_size;
    uint32_t s_blocks_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
};

typedef struct mext2_ext2_superblock_shortcut mext2_ext2_descriptor;

#endif
