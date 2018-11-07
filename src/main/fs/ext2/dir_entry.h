#ifndef MEXT2_FS_EXT2_DIRENTRY_H
#define MEXT2_FS_EXT2_DIRENTRY_H
#include "common.h"

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
