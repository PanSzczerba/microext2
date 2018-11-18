#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../file.h"
#include "file.h"
#include "sd.h"
#include "debug.h"
#include "common.h"
#include "ext2/inode.h"
#include "ext2/superblock.h"
#include "ext2/ext2.h"

STATIC uint32_t get_next_data_block_no(mext2_file* fd)
{
    uint32_t block_index = fd->fs_specific.ext2.pos_in_file / EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) + 1;
    return mext2_get_data_block_by_inode_address_index(fd->sd, fd->fs_specific.ext2.i_desc.inode_address, block_index);
}


uint8_t mext2_ext2_open(struct mext2_file* fd, char* path, uint8_t mode)
{
    mext2_bool create_if_not_found = MEXT2_FALSE;
    if(mode & MEXT2_WRITE)
        create_if_not_found = MEXT2_TRUE;
    uint32_t inode_number = mext2_get_regular_file_inode(fd->sd, path, create_if_not_found);
    if(inode_number == EXT2_INVALID_INO)
        return MEXT2_RETURN_FAILURE;

    fd->fs_specific.ext2.i_desc.inode_no = inode_number;
    struct mext2_ext2_inode* inode;
    if((fd->fs_specific.ext2.i_desc.inode_address = mext2_get_ext2_inode_address(fd->sd, inode_number)).block_address == EXT2_INVALID_BLOCK_NO)
    {
        mext2_error("Could not lookup inode %u address", inode_number);
        return MEXT2_RETURN_FAILURE;
    }

    if((inode = mext2_get_ext2_inode(fd->sd, fd->fs_specific.ext2.i_desc.inode_address)) == NULL)
    {
        mext2_error("Could not read inode %u, block %x", inode_number, fd->fs_specific.ext2.i_desc.inode_address);
        return MEXT2_RETURN_FAILURE;
    }

    if((inode->i_mode & EXT2_FORMAT_MASK) != EXT2_S_IFREG)
    {
        mext2_error("Could not open file: Not a regular file");
        mext2_debug("Inode mode: %#hx", inode->i_mode);
        return MEXT2_RETURN_FAILURE;
    }

    fd->fs_specific.ext2.pos_in_file = 0;
    fd->fs_specific.ext2.current_block = inode->i_direct_block[0];

    fd->mode = mode;
    if(mode & MEXT2_WRITE)
    {
        if(++fd->sd->fs.write_open_file_counter == 1)
        {
            struct mext2_ext2_superblock* superblock;
            if((superblock = mext2_get_main_ext2_superblock(fd->sd)) == NULL)
                return MEXT2_RETURN_FAILURE;

            mext2_debug("Superblock magic: %#hx", superblock->s_magic);

            superblock->s_state = mext2_cpu_to_le16(EXT2_ERROR_FS);
            if(mext2_update_ext2_superblocks_with_ptr(fd->sd, superblock) != MEXT2_RETURN_SUCCESS)
                return MEXT2_RETURN_FAILURE;
        }

        if(mode & MEXT2_APPEND)
        {
            fd->fs_specific.ext2.pos_in_file = fd->fs_specific.ext2.i_desc.i_size;
            fd->fs_specific.ext2.current_block = mext2_get_data_block_by_inode_address_index(fd->sd, fd->fs_specific.ext2.i_desc.inode_address, fd->fs_specific.ext2.pos_in_file / EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size));
        }
        else if(mode & MEXT2_TRUNCATE)
        {
            if(mext2_ext2_inode_truncate(fd->sd, fd->fs_specific.ext2.i_desc.inode_no, 0) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Truncating file error");
                return MEXT2_RETURN_FAILURE;
            }
        }
    }

    if((inode = mext2_get_ext2_inode(fd->sd, fd->fs_specific.ext2.i_desc.inode_address)) == NULL)
    {
        mext2_error("Could not read inode %u, block %x", inode_number, fd->fs_specific.ext2.i_desc.inode_address);
        return MEXT2_RETURN_FAILURE;
    }
    fd->fs_specific.ext2.i_desc.i_blocks = inode->i_blocks;
    fd->fs_specific.ext2.i_desc.i_size = (uint64_t)inode->i_size;

    if(fd->sd->fs.descriptor.ext2.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE)
        fd->fs_specific.ext2.i_desc.i_size  |= (((uint64_t)inode->i_dir_acl) << (sizeof(uint32_t) * BITS_IN_BYTE));

    mext2_debug("Opened file: %s, Inode: %u, File mode: %#hx, File size: %llu, Blocks: %u, Permissions: %#hx", path, inode_number, mode, fd->fs_specific.ext2.i_desc.i_size, fd->fs_specific.ext2.i_desc.i_blocks, inode->i_mode);

    return MEXT2_RETURN_SUCCESS;
}

uint8_t mext2_ext2_close(struct mext2_file* fd)
{
    if(fd->mode & MEXT2_WRITE)
    {
        if(--fd->sd->fs.write_open_file_counter == 0)
        {
            struct mext2_ext2_superblock* superblock;
            if((superblock = mext2_get_main_ext2_superblock(fd->sd)) == NULL)
                return MEXT2_RETURN_FAILURE;

            mext2_debug("Superblock magic: %#hx", superblock->s_magic);

            if(fd->sd->fs.fs_flags & MEXT2_FS_ERRORS)
                superblock->s_state = mext2_cpu_to_le16(EXT2_ERROR_FS);
            else
                superblock->s_state = mext2_cpu_to_le16(EXT2_VALID_FS);

            if(mext2_update_ext2_superblocks_with_ptr(fd->sd, superblock) != MEXT2_RETURN_SUCCESS)
                return MEXT2_RETURN_FAILURE;

            if(mext2_update_ext2_block_group_descriptors(fd->sd) != MEXT2_RETURN_SUCCESS)
                return MEXT2_RETURN_FAILURE;
        }
    }

    return MEXT2_RETURN_SUCCESS;
}

size_t mext2_ext2_write(struct mext2_file* fd, void* buffer, size_t count)
{

    uint64_t inode_size_needed = fd->fs_specific.ext2.pos_in_file + count;
    if(fd->fs_specific.ext2.i_desc.i_size < inode_size_needed)
    {
        if(mext2_ext2_inode_extend(fd->sd, fd->fs_specific.ext2.i_desc.inode_address, inode_size_needed) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Could not resize inode %u to size %llu", fd->fs_specific.ext2.i_desc.inode_no, inode_size_needed);
            return 0;
        }

        fd->fs_specific.ext2.i_desc.i_size = inode_size_needed;
        if(fd->fs_specific.ext2.current_block == EXT2_INVALID_BLOCK_NO
           && (fd->fs_specific.ext2.current_block
              = mext2_get_data_block_by_inode_address_index(fd->sd, fd->fs_specific.ext2.i_desc.inode_address,
                fd->fs_specific.ext2.pos_in_file / EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size)))
           == EXT2_INVALID_BLOCK_NO)
        {
            mext2_error("Could not fetch block number for inode %u index %u", fd->fs_specific.ext2.i_desc.inode_no,
                    fd->fs_specific.ext2.pos_in_file / EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size));
            return 0;
        }
    }

    size_t bytes_written = 0;
    uint32_t bytes_left_in_block = EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size)- (fd->fs_specific.ext2.pos_in_file % EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size));
    uint8_t* data_block = (uint8_t*)mext2_usefull_blocks;
    if((data_block = (uint8_t*)mext2_get_ext2_block(fd->sd, fd->fs_specific.ext2.current_block)) == NULL)
    {
        mext2_error("Could not read current data block");
        return 0;
    }

    while (bytes_left_in_block < count - bytes_written)
    {
        mext2_debug("Bytes left to write %zu", count - bytes_written);
        memcpy((void*)(data_block + EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) - bytes_left_in_block), (void*)((uint8_t*)buffer + bytes_written), bytes_left_in_block);
        mext2_debug("Copied %zu bytes", bytes_left_in_block);

        if(mext2_put_ext2_block(fd->sd, (block512_t*)data_block, fd->fs_specific.ext2.current_block) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Could not write data block %u", fd->fs_specific.ext2.current_block);
            return bytes_written;
        }

        bytes_written += bytes_left_in_block;
        mext2_errno = MEXT2_NO_ERRORS;
        if((fd->fs_specific.ext2.current_block = get_next_data_block_no(fd)) == EXT2_INVALID_BLOCK_NO)
        {
            if(mext2_errno != MEXT2_NO_ERRORS)
            {
                mext2_error("Could not read next data block number");
                return bytes_written;
            }
        }
        fd->fs_specific.ext2.pos_in_file += bytes_left_in_block;
        bytes_left_in_block = EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size);
    }

    memcpy((void*)(data_block + EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) - bytes_left_in_block), (void*)((uint8_t*)buffer + bytes_written), count - bytes_written);
    memset((void*)(data_block + EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) - bytes_left_in_block
            + (count - bytes_written)), 0, EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) - (count - bytes_written));
    if(mext2_put_ext2_block(fd->sd, (block512_t*)data_block, fd->fs_specific.ext2.current_block) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not write data block %u", fd->fs_specific.ext2.current_block);
        return 0;
    }
    mext2_debug("Copied %zu bytes", count - bytes_written);
    fd->fs_specific.ext2.pos_in_file += count - bytes_written;

    return count;
}

size_t mext2_ext2_read(struct mext2_file* fd, void* buffer, size_t count)
{
    mext2_debug("Reading %zu bytes, position in file: %llu, file size: %llu", count, fd->fs_specific.ext2.pos_in_file, fd->fs_specific.ext2.i_desc.i_size);
    if(count == 0)
        return 0;

    if(fd->fs_specific.ext2.pos_in_file == fd->fs_specific.ext2.i_desc.i_size)
    {
        mext2_errno = MEXT2_EOF;
        mext2_warning("Cannot read past the end of the file");
        return 0;
    }

    uint8_t* data_block;
    if((data_block = (uint8_t*)mext2_get_ext2_block(fd->sd, fd->fs_specific.ext2.current_block)) == NULL)
    {
        mext2_error("Could not read current data block");
        return 0;
    }

    uint64_t bytes_till_eof = fd->fs_specific.ext2.i_desc.i_size - fd->fs_specific.ext2.pos_in_file;

    size_t bytes_to_read;
    if((uint64_t)bytes_till_eof < count)
    {
        bytes_to_read = (size_t)bytes_till_eof;
        mext2_errno = MEXT2_EOF;
    }
    else
    {
        bytes_to_read = count;
    }
    mext2_debug("Bytes to read: %zu", bytes_to_read);

    size_t bytes_read = 0;

    uint32_t bytes_left_in_block = EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size)- (fd->fs_specific.ext2.pos_in_file % EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size));
    while (bytes_left_in_block < bytes_to_read - bytes_read)
    {
        mext2_debug("Bytes left to read %zu", bytes_to_read - bytes_read);
        memcpy((void*)((uint8_t*)buffer + bytes_read), (void*)(data_block + EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) - bytes_left_in_block), bytes_left_in_block);
        mext2_debug("Copied %zu bytes", bytes_left_in_block);

        mext2_errno = MEXT2_NO_ERRORS;
        if((fd->fs_specific.ext2.current_block = get_next_data_block_no(fd)) == EXT2_INVALID_BLOCK_NO)
        {
            if(mext2_errno != MEXT2_NO_ERRORS)
            {
                mext2_error("Could not read next data block number");
                return EXT2_INVALID_BLOCK_NO;
            }
        }
        bytes_read += bytes_left_in_block;
        fd->fs_specific.ext2.pos_in_file += bytes_left_in_block;

        if((data_block = (uint8_t*)mext2_get_ext2_block(fd->sd, fd->fs_specific.ext2.current_block)) == NULL)
        {
            mext2_error("Could not read data block");
            return 0;
        }
        bytes_left_in_block = EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size);

    }

    memcpy((void*)((uint8_t*)buffer + bytes_read), (void*)(data_block
            + EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) - bytes_left_in_block), bytes_to_read - bytes_read);
    mext2_debug("Copied %zu bytes", bytes_to_read - bytes_read);
    fd->fs_specific.ext2.pos_in_file += bytes_to_read - bytes_read;

    return bytes_to_read;
}

int mext2_ext2_seek(struct mext2_file* fd, int count)
{
    return MEXT2_RETURN_FAILURE;
}

mext2_bool mext2_ext2_eof(struct mext2_file* fd)
{
    return MEXT2_RETURN_FAILURE;
}
