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

#define BITS_IN_BYTE 8

STATIC uint32_t get_data_block_by_inode_index(mext2_file* fd, uint32_t block_index)
{
    mext2_errno = MEXT2_NO_ERRORS;
    struct mext2_ext2_inode* inode;
    if((inode = mext2_get_ext2_inode_by_no(fd->sd, fd->fs_specific.ext2.i_desc.inode_no)) == NULL)
    {
        return EXT2_INVALID_BLOCK_NO;
    }
/*
    mext2_debug("Inode block addresses: %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x",
            inode->i_direct_block[0],
            inode->i_direct_block[1],
            inode->i_direct_block[2],
            inode->i_direct_block[3],
            inode->i_direct_block[4],
            inode->i_direct_block[5],
            inode->i_direct_block[6],
            inode->i_direct_block[7],
            inode->i_direct_block[8],
            inode->i_direct_block[9],
            inode->i_direct_block[10],
            inode->i_direct_block[11],
            inode->i_indirect_block,
            inode->i_double_indirect_block,
            inode->i_triple_indirect_block
             );
 */

    uint32_t block_addresses_per_block = EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) / sizeof(uint32_t);
    uint32_t first_block_index_in_double_indirect = I_INDIRECT_BLOCK_INDEX + block_addresses_per_block;
    uint32_t first_block_index_in_triple_indirect = first_block_index_in_double_indirect + block_addresses_per_block * block_addresses_per_block;
    if(block_index < I_INDIRECT_BLOCK_INDEX)
    {
        mext2_debug("Block index: %u, Next data block at block: %u", block_index, mext2_le_to_cpu32(inode->i_direct_block[block_index]));
        return mext2_le_to_cpu32(inode->i_direct_block[block_index]);

    }

    uint32_t* addresses;
    if(block_index >= first_block_index_in_triple_indirect)
    {
        mext2_debug("Next data block in triple indirect block");
        uint32_t triple_indirect_index = (block_index - first_block_index_in_triple_indirect) / (block_addresses_per_block * block_addresses_per_block);
        uint32_t double_indirect_index = (block_index - first_block_index_in_triple_indirect) % (block_addresses_per_block * block_addresses_per_block) / block_addresses_per_block;
        uint32_t indirect_index = (block_index - first_block_index_in_triple_indirect) % (block_addresses_per_block * block_addresses_per_block) % block_addresses_per_block;

        if((addresses = (uint32_t*)mext2_get_ext2_block(fd->sd, mext2_le_to_cpu32(inode->i_triple_indirect_block))) == NULL)
        {
            mext2_error("Can't read triple_indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        if((addresses = (uint32_t*)mext2_get_ext2_block(fd->sd, mext2_le_to_cpu32(addresses[triple_indirect_index]))) == NULL)
        {
            mext2_error("Can't read double indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        if((addresses = (uint32_t*)mext2_get_ext2_block(fd->sd, mext2_le_to_cpu32(addresses[double_indirect_index]))) == NULL)
        {
            mext2_error("Can't read indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        mext2_debug("Block index: %u, Next data at block: %u", block_index, mext2_le_to_cpu32(addresses[indirect_index]));
        return mext2_le_to_cpu32(addresses[indirect_index]);
    }
    else if(block_index >= first_block_index_in_double_indirect)
    {
        mext2_debug("Next data block in double indirect block");
        uint32_t double_indirect_index = (block_index - first_block_index_in_double_indirect) / block_addresses_per_block;
        uint32_t indirect_index = (block_index - first_block_index_in_double_indirect) % block_addresses_per_block;

        if((addresses = (uint32_t*)mext2_get_ext2_block(fd->sd, mext2_le_to_cpu32(inode->i_double_indirect_block))) == NULL)
        {
            mext2_error("Can't read double block");
            return EXT2_INVALID_BLOCK_NO;
        }

        if((addresses = (uint32_t*)mext2_get_ext2_block(fd->sd, mext2_le_to_cpu32(addresses[double_indirect_index]))) == NULL)
        {
            mext2_error("Can't read indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        mext2_debug("Block index: %u, Next data at block: %u", block_index, mext2_le_to_cpu32(addresses[indirect_index]));
        return mext2_le_to_cpu32(addresses[indirect_index]);
    }
    else
    {
        mext2_debug("Next data block in indirect block");
        if((addresses = (uint32_t*)mext2_get_ext2_block(fd->sd, inode->i_indirect_block)) == NULL)
        {
            mext2_error("Can't read indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        mext2_debug("Block index: %u, Next data block at block: %u", block_index, mext2_le_to_cpu32(addresses[block_index - I_INDIRECT_BLOCK_INDEX]));
        return mext2_le_to_cpu32(addresses[block_index - I_INDIRECT_BLOCK_INDEX]);
    }

}

STATIC uint32_t get_next_data_block_no(mext2_file* fd)
{
    uint32_t block_index = fd->fs_specific.ext2.pos_in_file / EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size) + 1;
    return get_data_block_by_inode_index(fd, block_index);
}

uint8_t mext2_ext2_open(struct mext2_file* fd, char* path, uint8_t mode)
{
    uint32_t inode_number = mext2_inode_no_lookup(fd->sd, path);
    if(inode_number == EXT2_INVALID_INO)
        return MEXT2_RETURN_FAILURE;

    fd->fs_specific.ext2.i_desc.inode_no = inode_number;
    struct mext2_ext2_inode_address inode_address;
    struct mext2_ext2_inode* inode;
    if((inode_address = mext2_get_ext2_inode_address(fd->sd, inode_number)).block_address == EXT2_INVALID_BLOCK_NO)
    {
        mext2_error("Could not lookup inode %u address", inode_number);
        return MEXT2_RETURN_FAILURE;
    }

    if((inode = mext2_get_ext2_inode(fd->sd, inode_address)) == NULL)
    {
        mext2_error("Could not read inode %u, block %x", inode_number, inode_address.block_address);
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
            if(mext2_update_ext2_main_superblock_with_ptr(fd->sd, superblock) != MEXT2_RETURN_SUCCESS)
                return MEXT2_RETURN_FAILURE;
        }

        if(mode & MEXT2_APPEND)
        {
            fd->fs_specific.ext2.pos_in_file = fd->fs_specific.ext2.i_desc.i_size;
            fd->fs_specific.ext2.current_block = get_data_block_by_inode_index(fd, fd->fs_specific.ext2.pos_in_file / EXT2_BLOCK_SIZE(fd->sd->fs.descriptor.ext2.s_log_block_size));
        }
        else if(mode & MEXT2_TRUNCATE)
        {
            if(mext2_ext2_truncate(fd->sd, fd->fs_specific.ext2.i_desc.inode_no, 0) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Truncating file error");
                return MEXT2_RETURN_FAILURE;
            }

        }
    }

    if((inode = mext2_get_ext2_inode(fd->sd, inode_address)) == NULL)
    {
        mext2_error("Could not read inode %u, block %x", inode_number, inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }
    fd->fs_specific.ext2.i_desc.i_blocks = inode->i_blocks;
    fd->fs_specific.ext2.i_desc.i_size = (uint64_t)inode->i_size;

    if(fd->sd->fs.descriptor.ext2.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE)
        fd->fs_specific.ext2.i_desc.i_size  |= (((uint64_t)inode->i_dir_acl) << (sizeof(uint32_t) * BITS_IN_BYTE));

    mext2_debug("Opened file: %s, Inode: %u, File mode: %#hx, File size: %llu, Blocks %u", path, inode_number, mode, fd->fs_specific.ext2.i_desc.i_size, fd->fs_specific.ext2.i_desc.i_blocks);

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

            if(fd->sd->fs.fs_flags & MEXT2_ERRONEOUS_FS)
                superblock->s_state = mext2_cpu_to_le16(EXT2_ERROR_FS);
            else
                superblock->s_state = mext2_cpu_to_le16(EXT2_VALID_FS);

            if(mext2_update_ext2_main_superblock_with_ptr(fd->sd, superblock) != MEXT2_RETURN_SUCCESS)
                return MEXT2_RETURN_FAILURE;
        }
    }

    return MEXT2_RETURN_SUCCESS;
}

#define FIRST_BLOCK_INDEX_IN_DOUBLE_INDIRECT(s_log_block_size) (I_INDIRECT_BLOCK_INDEX + (EXT2_BLOCK_SIZE(s_log_block_size) / uint32_t) * (EXT2_BLOCK_SIZE(s_log_block_size) / uint32_t))
#define FIRST_BLOCK_INDEX_IN_TRIPLE_INDIRECT(s_log_block_size) (I_INDIRECT_BLOCK_INDEX + (EXT2_BLOCK_SIZE(s_log_block_size) / uint32_t) * (EXT2_BLOCK_SIZE(s_log_block_size) / uint32_t) * (EXT2_BLOCK_SIZE(s_log_block_size) / uint32_t))

size_t mext2_ext2_write(struct mext2_file* fd, void* buffer, size_t count)
{
    return MEXT2_RETURN_FAILURE;
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

    memcpy((void*)((uint8_t*)buffer + bytes_read), (void*)data_block, bytes_to_read - bytes_read);
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
