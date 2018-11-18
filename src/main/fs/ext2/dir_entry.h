#ifndef MEXT2_FS_EXT2_DIRENTRY_H
#define MEXT2_FS_EXT2_DIRENTRY_H
#include "common.h"

#define EXT2_FT_UNKNOWN  0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR      2
#define EXT2_FT_CHRDEV   3
#define EXT2_FT_BLKDEV   4
#define EXT2_FT_FIFO     5
#define EXT2_FT_SOCK     6
#define EXT2_FT_SYMLINK  7

struct mext2_ext2_dir_entry_head
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
} __mext2_packed;

struct mext2_ext2_dir_entry
{
    struct mext2_ext2_dir_entry_head head;
    char name[255];
} __mext2_packed;

#endif
