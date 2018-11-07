#ifndef MEXT2_FS_EXT2_BLOCK_GROUP_DESCRIPTOR_H
#define MEXT2_FS_EXT2_BLOCK_GROUP_DESCRIPTOR_H
struct mext2_ext2_block_group_descriptor
{
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];

};

#endif
