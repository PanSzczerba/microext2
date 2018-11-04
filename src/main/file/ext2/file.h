#ifndef MEXT2_FILE_EXT2_H
#define MEXT2_FILE_EXT2_H
#include <stddef.h>
#include "ext2/inode_descriptor.h"

struct mext2_sd;
struct mext2_file;

struct mext2_ext2_file
{
    struct mext2_inode_descriptor i_desc;
};

struct mext2_file* mext2_ext2_open(struct mext2_sd* sd, char* path, uint16_t mode);
uint8_t mext2_ext2_close(struct mext2_file* fd, int count);
uint8_t mext2_ext2_write(struct mext2_file* fd, void* buffer, size_t count);
uint8_t mext2_ext2_read(struct mext2_file* fd, void* buffer, size_t count);
uint8_t mext2_ext2_seek(struct mext2_file* fd, int count);
uint8_t mext2_ext2_eof(struct mext2_file* fd);

#endif
