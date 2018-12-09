#ifndef MEXT2_FILE_EXT2_H
#define MEXT2_FILE_EXT2_H
#include <stddef.h>
#include <stdint.h>
#include "common.h"
#include "ext2/inode_descriptor.h"

struct mext2_sd;
struct mext2_file;

struct mext2_ext2_file
{
    uint64_t pos_in_file;
    uint32_t current_block;
    struct mext2_inode_descriptor i_desc;
};

uint8_t mext2_ext2_open(struct mext2_file* fd, char* path, uint8_t mode);
uint8_t mext2_ext2_close(struct mext2_file* fd);
size_t mext2_ext2_write(struct mext2_file* fd, void* buffer, size_t count);
size_t mext2_ext2_read(struct mext2_file* fd, void* buffer, size_t count);
int mext2_ext2_seek(struct mext2_file* fd, uint8_t seek_mode, int count);
mext2_bool mext2_ext2_eof(struct mext2_file* fd);

#endif
