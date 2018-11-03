#ifndef MEXT2_FS_EXT2_H
#define MEXT2_FS_EXT2_H

struct mext2_inode_descriptor
{
    uint64_t inode_pos;
    uint32_t i_size;
    uint32_t i_blocks;
    uint32_t i_dir_acl;
} __mext2_packed;

#endif
