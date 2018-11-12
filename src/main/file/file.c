#include <stdint.h>
#include "debug.h"
#include "file.h"
#include "sd.h"
#include "limit.h"
#include "common.h"

#define BITS_IN_BYTE 8
#define FILE_DESC_BITMAP_SIZE ((MEXT2_MAX_FILE_NUMBER + BITS_IN_BYTE - 1) / BITS_IN_BYTE)

STATIC mext2_file file_pool[MEXT2_MAX_FILE_NUMBER];
STATIC uint8_t file_pool_bitmap[FILE_DESC_BITMAP_SIZE];

STATIC mext2_file* reserve_fd()
{
    uint16_t i;
    for(i = 0; i < MEXT2_MAX_FILE_NUMBER; i++)
    {
        if((file_pool_bitmap[i / BITS_IN_BYTE] & (1 << (i % BITS_IN_BYTE))) == 0)
        {
            file_pool_bitmap[i / BITS_IN_BYTE] |= (1 << (i % BITS_IN_BYTE)); // mark as reserved
            break;
        }
    }

    if(i == MEXT2_MAX_FILE_NUMBER)
    {
        mext2_errno = MEXT2_NOT_ENOUGH_FD;
        return NULL; // no available file descriptors found
    }
    else
        return &file_pool[i];
}

STATIC void free_fd(mext2_file* fd)
{
    uint16_t index = (fd - file_pool);
    file_pool_bitmap[index / BITS_IN_BYTE] &= ~(1 << (index % BITS_IN_BYTE));
}

mext2_file* mext2_open(struct mext2_sd* sd, char* path, uint8_t mode)
{
    if(sd->fs.fs_flags & MEXT2_FS_ERRORS)
    {
        mext2_warning("Filesytem marked as containing errors");
//      mext2_errno = MEXT2_ERRONEOUS_FS;
//      return NULL; //same as in ext2.c
    }

    if((sd->fs.fs_flags & MEXT2_READ_ONLY) && mode != MEXT2_READ)
    {
        mext2_error("Filesystem marked as read only, cannot open for writing");
        mext2_errno = MEXT2_READ_ONLY_FS;
        return NULL;
    }

    mext2_file* fd = reserve_fd();
    if(fd == NULL)
        return NULL;

    fd->sd = sd;
    if(sd->fs.open_strategy(fd, path, mode) != MEXT2_RETURN_SUCCESS)
    {
        free_fd(fd);
        return NULL;
    }
    else
    {
        return fd;
    }
}

uint8_t mext2_close(mext2_file* fd)
{
    if(fd->sd->fs.close_strategy(fd) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not close file");
        return MEXT2_RETURN_FAILURE;
    }
    else
    {
        free_fd(fd);
        mext2_log("Successfully closed file");
        return MEXT2_RETURN_SUCCESS;
    }
}

size_t mext2_write(mext2_file* fd, void* buffer, size_t count)
{
    if((fd->mode & MEXT2_WRITE) == 0)
    {
        mext2_error("Can't write to file opened in read-only mode");
        mext2_errno = MEXT2_RO_WRITE_TRY;
        return 0;
    }
    else
        return fd->sd->fs.write_strategy(fd, buffer, count);
}

size_t mext2_read(mext2_file* fd, void* buffer, size_t count)
{
    return fd->sd->fs.read_strategy(fd, buffer, count);
}

int mext2_seek(mext2_file* fd, int count)
{
    return fd->sd->fs.seek_strategy(fd, count);
}

mext2_bool mext2_eof(mext2_file* fd)
{
    return fd->sd->fs.eof_strategy(fd);
}
