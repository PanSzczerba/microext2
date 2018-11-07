#include <stddef.h>
#include <stdint.h>
#include "../file.h"
#include "file.h"
#include "sd.h"
#include "debug.h"
#include "common.h"
#include "ext2/inode.h"
#include "ext2/superblock.h"
#include "ext2/ext2.h"

#define BITS_IN_BYTE 8

STATIC void fill_fd_from_inode(struct mext2_file* fd, struct mext2_ext2_inode* inode)
{

    fd->fs_specific.ext2.i_desc.i_blocks = inode->i_blocks;
    fd->fs_specific.ext2.i_desc.i_size = (uint64_t)inode->i_size;

    if(fd->sd->fs.descriptor.ext2.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE)
        fd->fs_specific.ext2.i_desc.i_size  |= (((uint64_t)inode->i_dir_acl) << (sizeof(uint32_t) * BITS_IN_BYTE));
}

uint8_t mext2_ext2_open(struct mext2_file* fd, char* path, uint8_t mode)
{
    uint32_t inode_number = mext2_inode_no_lookup(fd->sd, path);
    if(inode_number == EXT2_INVALID_INO)
    {
        switch(ext2_errno)
        {
        case EXT2_INVALID_PATH:
            mext2_error("Could not open file: Invalid path");
            break;
        case EXT2_READ_ERROR:
            mext2_error("Could not open file: Read error, check SD card reader device");
            break;
        default:
            mext2_error("Unknown error have occured");
        }
        return MEXT2_RETURN_FAILURE;
    }

    fd->fs_specific.ext2.i_desc.inode_no = inode_number;
    struct mext2_ext2_inode* inode = mext2_get_ext2_inode(fd->sd, inode_number);

    if(mext2_is_big_endian())
        mext2_correct_inode_endianess(inode);

    if((inode->i_mode & EXT2_FORMAT_MASK) != EXT2_S_IFREG)
    {
        mext2_error("Could not open file: Not a regular file");
        mext2_debug("Inode mode: %#hx", inode->i_mode);
        return MEXT2_RETURN_FAILURE;
    }

    fill_fd_from_inode(fd, inode);
    fd->mode = mode;
    if((mode & MEXT2_WRITE) && (mode & MEXT2_APPEND))
    {
        fd->fs_specific.ext2.pos_in_file = fd->fs_specific.ext2.i_desc.i_size;
    }
    else
    {
        fd->fs_specific.ext2.pos_in_file = 0;
    }
    mext2_debug("Opened file: %s, Inode: %d, File mode: %#hx, File size: %llu", path, inode_number, mode, fd->fs_specific.ext2.i_desc.i_size);

    return MEXT2_RETURN_SUCCESS;
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
