#ifndef MEXT2_FS_EXT2_DIRENTRY_H
#define MEXT2_FS_EXT2_DIRENTRY_H

struct direntry
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[255];
};

#endif
