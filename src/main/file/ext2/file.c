#include <stddef.h>
#include "file.h"

STATIC uint32_t inode_lookup(struct mext2_file* fd, char* path)
{
    return 0;
}

uint8_t mext2_ext2_open(struct mext2_file* fd, char* path, uint16_t mode)
{
    return MEXT2_RETURN_FAILURE;
}

uint8_t mext2_ext2_close(struct mext2_file* fd)
{
    return MEXT2_RETURN_FAILURE;
}

uint8_t mext2_ext2_write(struct mext2_file* fd, void* buffer, size_t count)
{
    return MEXT2_RETURN_FAILURE;
}

uint8_t mext2_ext2_read(struct mext2_file* fd, void* buffer, size_t count)
{
    return MEXT2_RETURN_FAILURE;
}

uint8_t mext2_ext2_seek(struct mext2_file* fd, int count)
{
    return MEXT2_RETURN_FAILURE;
}

uint8_t mext2_ext2_eof(struct mext2_file* fd)
{
    return MEXT2_RETURN_FAILURE;
}
