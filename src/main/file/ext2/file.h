#ifndef MEXT2_FILE_EXT2_H
#define MEXT2_FILE_EXT2_H
#include "ext2/inode_descriptor.h"

struct mext2_ext2_file
{
    struct mext2_inode_descriptor i_desc;
};

mext2_file* mext2_ext2_open(mext2_sd* sd, char* path, enum mext2_file_mode mode);
uint8_t mext2_ext2_close(mext2_file* fd, int count);
uint8_t mext2_ext2_write(mext2_file* fd, void* buffer, size_t count);
uint8_t mext2_ext2_read(mext2_file* fd, void* buffer, size_t count);
uint8_t mext2_ext2_seek(mext2_file* fd, int count);
uint8_t mext2_ext2_eof(mext2_file* fd);

#endif
