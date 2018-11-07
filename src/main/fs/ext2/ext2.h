#ifndef MEXT2_FS_EXT2_H
#define MEXT2_FS_EXT2_H

#include <stdint.h>

#define EXT2_BLOCK_SIZE(s_log_block_size) (1024 << s_log_block_size)
#define EXT2_BLOCK_GROUP_DESCRIPTOR_BLOCK_OFFSET 1

struct mext2_ext2_superblock;
struct mext2_sd;
struct block512_t;

enum ext2_errors
{
    EXT2_NO_ERRORS = 0,
    EXT2_INVALID_PATH,
    EXT2_READ_ERROR
};

extern uint8_t ext2_errno;

uint8_t mext2_ext2_sd_parser(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock);
uint32_t mext2_inode_no_lookup(struct mext2_sd* sd, char* path); // path needs to start with '/'
uint32_t mext2_inode_no_lookup_from_dir_inode(struct mext2_sd* sd, uint32_t dir_inode_no, char* name); // name must not contain slashes
struct mext2_ext2_inode* mext2_get_ext2_inode(struct mext2_sd* sd, uint32_t inode_no);
struct block512_t* mext2_get_ext2_block(struct mext2_sd* sd, uint32_t block_no);

#endif
