#include <string.h>
#include <stdlib.h>
#include "ext2.h"
#include "superblock.h"
#include "inode.h"
#include "block_group_descriptor.h"
#include "sd.h"
#include "dir_entry.h"
#include "common.h"
#include "debug.h"
#include "superblock.h"
#include "ext2/file.h"

#define EXT2_BLOCKS_TO_PHYSICAL(block_count, s_log_block_size) ((block_count) * (2 << (s_log_block_size)))

#define BLOCK_GROUP_DESCRIPTOR_TABLE_FIRST_BLOCK_OFFSET 1

#define BLOCK_GROUP_BY_BLOCK_NO(block_no, s_blocks_per_group) ((block_no) / (s_blocks_per_group))

#define BLOCK_GROUP_BLOCK_NO_INDEX(block_no, s_blocks_per_group) ((block_no) % (s_blocks_per_group))

#define BGD_BLOCK_NO(block_group_no, ext2_block_size, first_data_block) \
    ((block_group_no) \
    / (ext2_block_size / sizeof(struct mext2_ext2_block_group_descriptor)) \
    + (first_data_block) + BLOCK_GROUP_DESCRIPTOR_TABLE_FIRST_BLOCK_OFFSET)

#define BGD_BLOCK_OFFSET(block_group_no, ext2_block_size) \
    ((block_group_no) \
    % ((ext2_block_size) / sizeof(struct mext2_ext2_block_group_descriptor)))

enum block_indirection_level
{
    DIRECT_BLOCK = 0,
    INDIRECT_BLOCK,
    DOUBLE_INDIRECT_BLOCK,
    TRIPLE_INDIRECT_BLOCK,
};

STATIC uint32_t direct_block_addresses(uint16_t block_addresses_per_block, uint8_t indirection_level)
{
    uint32_t direct_blocks = 1;
    for(uint8_t i = DIRECT_BLOCK; i < indirection_level; i++)
        direct_blocks *= block_addresses_per_block;
    return direct_blocks;
}

STATIC void flip_block_group_descriptor_endianess(struct mext2_ext2_block_group_descriptor* bgd)
{
    bgd->bg_block_bitmap = mext2_flip_endianess32(bgd->bg_block_bitmap);
    bgd->bg_inode_bitmap = mext2_flip_endianess32(bgd->bg_inode_bitmap);
    bgd->bg_inode_table = mext2_flip_endianess32(bgd->bg_inode_table);
    bgd->bg_free_blocks_count = mext2_flip_endianess16(bgd->bg_free_blocks_count);
    bgd->bg_free_inodes_count = mext2_flip_endianess16(bgd->bg_free_inodes_count);
    bgd->bg_used_dirs_count = mext2_flip_endianess16(bgd->bg_used_dirs_count);
    bgd->bg_pad = mext2_flip_endianess16(bgd->bg_pad);
}

STATIC void flip_inode_endianess(struct mext2_ext2_inode* inode)
{
    inode->i_mode = mext2_flip_endianess16(inode->i_mode);
    inode->i_uid = mext2_flip_endianess16(inode->i_uid);
    inode->i_size = mext2_flip_endianess32(inode->i_size);
    inode->i_atime = mext2_flip_endianess32(inode->i_atime);
    inode->i_ctime = mext2_flip_endianess32(inode->i_ctime);
    inode->i_mtime = mext2_flip_endianess32(inode->i_mtime);
    inode->i_dtime = mext2_flip_endianess32(inode->i_dtime);
    inode->i_gid = mext2_flip_endianess16(inode->i_gid);
    inode->i_links_count = mext2_flip_endianess16(inode->i_links_count);
    inode->i_blocks = mext2_flip_endianess32(inode->i_blocks);
    inode->i_flags = mext2_flip_endianess32(inode->i_flags);
    inode->i_osd1 = mext2_flip_endianess32(inode->i_osd1);
    for(uint8_t i = 0; i < sizeof(inode->i_direct_block)/sizeof(inode->i_direct_block[0]); i++)
        inode->i_direct_block[i] = mext2_flip_endianess32(inode->i_direct_block[i]);
    inode->i_indirect_block = mext2_flip_endianess32(inode->i_indirect_block);
    inode->i_double_indirect_block = mext2_flip_endianess32(inode->i_double_indirect_block);
    inode->i_triple_indirect_block = mext2_flip_endianess32(inode->i_triple_indirect_block);
    inode->i_generation = mext2_flip_endianess32(inode->i_generation);
    inode->i_file_acl = mext2_flip_endianess32(inode->i_file_acl);
    inode->i_dir_acl = mext2_flip_endianess32(inode->i_dir_acl);
    inode->i_faddr = mext2_flip_endianess32(inode->i_faddr);
}

STATIC int compare_block_numbers (const void * block1, const void * block2)
{
  if      ( *(uint32_t*)block1 <  *(uint32_t*)block2 ) return -1;
  else if ( *(uint32_t*)block1 == *(uint32_t*)block2 ) return  0;
  else return 1;
}

STATIC uint16_t mext2_allocate_blocks_one_group(mext2_sd* sd, uint16_t block_group_no, uint32_t* block_list, uint16_t blocks_to_allocate)
{
    const uint32_t ext2_block_size = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size);
    block512_t* block;
    struct mext2_ext2_block_group_descriptor* block_group_descriptor;
    if((block = mext2_get_ext2_block(
            sd,
            BGD_BLOCK_NO(block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)))
        == NULL)
    {
        mext2_error("Could not read block group descriptor");
        return 0;
    }

    block_group_descriptor = (struct mext2_ext2_block_group_descriptor*)block;
    block_group_descriptor += BGD_BLOCK_OFFSET(block_group_no, ext2_block_size);

    uint32_t block_bitmap_block = mext2_le_to_cpu32(block_group_descriptor->bg_block_bitmap);
    uint8_t* block_bitmap;

    if((block_bitmap = (uint8_t*)mext2_get_ext2_block(sd, block_bitmap_block)) == NULL)
    {
        mext2_error("Cannot fetch block bitmap at block %u", block_bitmap_block);
        return 0;
    }

    uint32_t first_block_in_block_group_block = block_group_no * sd->fs.descriptor.ext2.s_blocks_per_group;

    uint16_t blocks_allocated = 0;
    uint32_t blocks_in_bitmap = sd->fs.descriptor.ext2.s_blocks_per_group;
    for(uint32_t i = 0; i < blocks_in_bitmap && blocks_allocated < blocks_to_allocate; i++)
    {
        if(i == ext2_block_size * BITS_IN_BYTE)
        {
            if(mext2_put_ext2_block(sd, block, block_bitmap_block) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Can't update block bitmap");
                return MEXT2_RETURN_FAILURE;
            }

            if((block_bitmap = (uint8_t*)mext2_get_ext2_block(sd, ++block_bitmap_block)) == NULL)
            {
                mext2_error("Cannot fetch block bitmap at block %u", block_bitmap_block);
                return 0;
            }

            i = 0;
            blocks_in_bitmap -= ext2_block_size * BITS_IN_BYTE;
            first_block_in_block_group_block += ext2_block_size * BITS_IN_BYTE;
        }

        if(!(block_bitmap[i / BITS_IN_BYTE] & (1 << (i % BITS_IN_BYTE))))
        {
            block_list[blocks_allocated++] = first_block_in_block_group_block + i;
            mext2_debug("Successfully allocated block %u", first_block_in_block_group_block + i);
            block_bitmap[i / BITS_IN_BYTE] |= (1 << (i % BITS_IN_BYTE));
        }
    }

    if(mext2_put_ext2_block(sd, block, block_bitmap_block) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Can't update block bitmap");
        return MEXT2_RETURN_FAILURE;
    }

    if((block = mext2_get_ext2_block(
            sd,
            BGD_BLOCK_NO(block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)))
        == NULL)
    {
        mext2_error("Could not read block group descriptor");
        return 0;
    }

    block_group_descriptor = (struct mext2_ext2_block_group_descriptor*)block;
    block_group_descriptor += BGD_BLOCK_OFFSET(block_group_no, ext2_block_size);

    block_group_descriptor->bg_free_blocks_count = mext2_cpu_to_le16(mext2_le_to_cpu16(block_group_descriptor->bg_free_blocks_count) - blocks_allocated);
    if(mext2_put_ext2_block(sd, block,
            BGD_BLOCK_NO(block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not update block group descriptor no %u, at block %u", block_group_no, BGD_BLOCK_NO(block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block));
        return blocks_allocated;
    }

    mext2_debug("Allocated %hu blocks in group %hu", blocks_allocated, block_group_no);
    return blocks_allocated;
}

uint16_t mext2_allocate_blocks(struct mext2_sd* sd, uint16_t primary_block_group_no, uint32_t* block_list, uint16_t blocks_to_allocate)
{
    uint16_t allocated_blocks;
    if((allocated_blocks = mext2_allocate_blocks_one_group(sd, primary_block_group_no, block_list, blocks_to_allocate)) < blocks_to_allocate)
    {
        uint16_t remaining_blocks_to_allocate = blocks_to_allocate - allocated_blocks;
        uint32_t* block_list_ptr = block_list + allocated_blocks;
        uint16_t block_groups_count = (sd->fs.descriptor.ext2.s_blocks_count + sd->fs.descriptor.ext2.s_blocks_per_group - 1) / sd->fs.descriptor.ext2.s_blocks_per_group;
        for(uint16_t i = 0; i < block_groups_count && mext2_errno == MEXT2_NO_ERRORS; i++)
        {
            if(i == primary_block_group_no)
                continue;

            if((allocated_blocks += mext2_allocate_blocks_one_group(sd, i, block_list_ptr, remaining_blocks_to_allocate)) == blocks_to_allocate)
            {
                break;
            }
            else
            {
                remaining_blocks_to_allocate = blocks_to_allocate - allocated_blocks;
                block_list_ptr = block_list + allocated_blocks;
            }
        }

        if(mext2_errno != MEXT2_NO_ERRORS)
            return 0;
    }

    // udate superblock
    if(allocated_blocks != 0)
    {
        mext2_debug("Allocated %hu blocks in all groups", allocated_blocks);
        sd->fs.descriptor.ext2.s_free_blocks_count -= allocated_blocks;
        mext2_update_ext2_main_superblock(sd);
    }

    return allocated_blocks;
}

uint8_t mext2_deallocate_blocks(struct mext2_sd* sd, uint32_t* block_list, uint16_t block_list_size)
{
    const uint32_t ext2_block_size = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size);
    qsort((void*)block_list, block_list_size, sizeof(uint32_t), compare_block_numbers);

    uint32_t i = 0;
    block512_t* block;
    struct mext2_ext2_block_group_descriptor* block_group_descriptor;
    mext2_bool deallocate_fail = MEXT2_FALSE;
    uint16_t blocks_deallocated = 0;

    while(!deallocate_fail && i < block_list_size)
    {

        if(block_list[i] >= sd->fs.descriptor.ext2.s_blocks_count)
        {
            mext2_error("Attempting to dealocate blocks starting from %u, while last block in filesystem is %u", block_list[i], sd->fs.descriptor.ext2.s_blocks_count);
            mext2_errno = MEXT2_INVALID_BLOCK_NO;
            return MEXT2_RETURN_FAILURE;
        }

        uint16_t current_block_group_no = BLOCK_GROUP_BY_BLOCK_NO(block_list[i], sd->fs.descriptor.ext2.s_blocks_per_group);
        uint16_t blocks_dealocated_current_group = 0;

        if((block = mext2_get_ext2_block(
                sd,
                BGD_BLOCK_NO(current_block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)))
            == NULL)
        {
            mext2_error("Could not read block group descriptor");
            return MEXT2_RETURN_FAILURE;
        }

        block_group_descriptor = (struct mext2_ext2_block_group_descriptor*)block;
        block_group_descriptor += BGD_BLOCK_OFFSET(current_block_group_no, ext2_block_size);

        uint32_t block_bitmap_fst_block_no = mext2_le_to_cpu32(block_group_descriptor->bg_block_bitmap);
        uint16_t block_bitmap_current_block = BLOCK_GROUP_BLOCK_NO_INDEX(block_list[i], sd->fs.descriptor.ext2.s_blocks_per_group)
                / (ext2_block_size * BITS_IN_BYTE);
        uint8_t* block_bitmap;


        if((block_bitmap = (uint8_t*)mext2_get_ext2_block(sd, block_bitmap_fst_block_no + block_bitmap_current_block)) == NULL)
        {
            mext2_error("Can't read block bitmap");
            return MEXT2_RETURN_FAILURE;
        }

        uint16_t block_bitmap_block = block_bitmap_current_block;
        uint32_t block_bitmap_index;
        do
        {
            if(block_list[i] >= sd->fs.descriptor.ext2.s_blocks_count)
            {
                mext2_error("Attempting to dealocate block no %u, while last block in filesystem is %u", block_list[i], sd->fs.descriptor.ext2.s_blocks_count);
                deallocate_fail = MEXT2_TRUE;
                mext2_errno = MEXT2_INVALID_BLOCK_NO;
                break;
            }

            block_bitmap_block = BLOCK_GROUP_BLOCK_NO_INDEX(block_list[i], sd->fs.descriptor.ext2.s_blocks_per_group)
                / (ext2_block_size * BITS_IN_BYTE);

            block_bitmap_index = BLOCK_GROUP_BLOCK_NO_INDEX(block_list[i], sd->fs.descriptor.ext2.s_blocks_per_group)
                % (ext2_block_size * BITS_IN_BYTE);

            if(block_bitmap_block != block_bitmap_current_block)
            {
                if(mext2_put_ext2_block(sd, (block512_t*)block_bitmap, block_bitmap_fst_block_no + block_bitmap_current_block) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Couldn't write block bitmap block");
                    return MEXT2_RETURN_FAILURE;
                }

                block_bitmap_current_block = block_bitmap_block;
                if((block_bitmap = (uint8_t*)mext2_get_ext2_block(sd, block_bitmap_fst_block_no + block_bitmap_current_block)) == NULL)
                {
                    mext2_error("Can't read block bitmap");
                    return MEXT2_RETURN_FAILURE;
                }
            }

            if(!(block_bitmap[block_bitmap_index / BITS_IN_BYTE] & (1 << (block_bitmap_index % BITS_IN_BYTE))))
            {
                mext2_warning("Block %u for deallocation was not set as allocated", block_list[i]);
            }
            else
            {
                block_bitmap[block_bitmap_index / BITS_IN_BYTE] &= ~(1 << (block_bitmap_index % BITS_IN_BYTE));
                mext2_log("Succesfully deallocated block %u", block_list[i]);
                blocks_dealocated_current_group++;
            }

        } while(++i < block_list_size && BLOCK_GROUP_BY_BLOCK_NO(block_list[i], sd->fs.descriptor.ext2.s_blocks_per_group) == current_block_group_no);

        if(mext2_put_ext2_block(sd, (block512_t*)block_bitmap, block_bitmap_fst_block_no + block_bitmap_current_block) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Couldn't write block bitmap block");
            return MEXT2_RETURN_FAILURE;
        }

        // update block group descriptor
        if(blocks_dealocated_current_group != 0)
        {
            if((block = mext2_get_ext2_block(
                    sd,
                    BGD_BLOCK_NO(current_block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)))
                == NULL)
            {
                mext2_error("Could not read block group descriptor");
                return MEXT2_RETURN_FAILURE;
            }

            block_group_descriptor = (struct mext2_ext2_block_group_descriptor*)block;
            block_group_descriptor += BGD_BLOCK_OFFSET(current_block_group_no, ext2_block_size);
            if(mext2_is_big_endian())
            {
                mext2_debug("Free blocks count group %u: %u, blocks deallocated : %u", current_block_group_no, mext2_flip_endianess16(block_group_descriptor->bg_free_blocks_count), blocks_dealocated_current_group);
                block_group_descriptor->bg_free_blocks_count = mext2_flip_endianess16(mext2_flip_endianess16(block_group_descriptor->bg_free_blocks_count) + blocks_dealocated_current_group);
            }
            else
            {
                mext2_debug("Free blocks count group %u: %u, blocks deallocated: %u", current_block_group_no, block_group_descriptor->bg_free_blocks_count, blocks_dealocated_current_group);
                block_group_descriptor->bg_free_blocks_count += blocks_dealocated_current_group;
            }

            if(mext2_put_ext2_block(sd, block, BGD_BLOCK_NO(current_block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Couldn't write block group descriptor");
                return MEXT2_RETURN_FAILURE;
            }
            blocks_deallocated += blocks_dealocated_current_group;
        }
    }

    // udate superblock
    if(blocks_deallocated != 0)
    {
        mext2_debug("Deallocated %u blocks", blocks_deallocated);
        sd->fs.descriptor.ext2.s_free_blocks_count += blocks_deallocated;
        mext2_update_ext2_main_superblock(sd);
    }

    if(deallocate_fail)
        return MEXT2_RETURN_FAILURE;
    else
        return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t free_blocks_with_list(struct mext2_sd* sd, uint32_t block_no, uint32_t offset, uint8_t indirection_level, uint32_t* blocks_to_deallocate_list, uint16_t max_block_list_size, uint16_t* current_list_size)
{
    mext2_debug("Running deallocate blocks, block number: %u, level %hhu, offset %u", block_no, indirection_level, offset);
    const uint32_t block_addresses_per_block = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(uint32_t);
    if(block_no == EXT2_INVALID_BLOCK_NO)
    {
        mext2_debug("Attempt to free block 0");
        return MEXT2_RETURN_SUCCESS;
    }

    uint32_t* block_numbers;
    uint32_t direct_addresses_per_address = direct_block_addresses(block_addresses_per_block, indirection_level - 1);
    uint32_t offset_fst_level_down = offset % direct_addresses_per_address;
    if(indirection_level > INDIRECT_BLOCK)
    {
        uint16_t i = offset / direct_addresses_per_address;

        if(offset_fst_level_down != 0)
        {
            if((block_numbers = (uint32_t*)mext2_get_ext2_block(sd, block_no)) == NULL)
            {
                mext2_error("Could not read block %u", block_no);
                return MEXT2_RETURN_FAILURE;
            }

            free_blocks_with_list(sd, mext2_le_to_cpu32(block_numbers[i]), offset_fst_level_down, indirection_level - 1, blocks_to_deallocate_list, max_block_list_size, current_list_size);
            i++;
        }

        for(; i < block_addresses_per_block && block_numbers[i] != 0; i++)
        {

            if((block_numbers = (uint32_t*)mext2_get_ext2_block(sd, block_no)) == NULL)
            {
                mext2_error("Could not read block %u", block_no);
                return MEXT2_RETURN_FAILURE;
            }

            if(free_blocks_with_list(sd, mext2_le_to_cpu32(block_numbers[i]), 0, indirection_level - 1, blocks_to_deallocate_list, max_block_list_size, current_list_size) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Couldn't free block: %u, indirection level %u", mext2_le_to_cpu32(block_numbers[i]), indirection_level - 1);
                return MEXT2_RETURN_FAILURE;
            }

        }
    }
    else
    {
        if((block_numbers = (uint32_t*)mext2_get_ext2_block(sd, block_no)) == NULL)
        {
            mext2_error("Could not read block %u", block_no);
            return MEXT2_RETURN_FAILURE;
        }

        for(uint16_t i = offset; i < block_addresses_per_block && block_numbers[i] != 0; i++)
        {

            mext2_debug("Adding to list block no %u", mext2_le_to_cpu32(block_numbers[i]));
            blocks_to_deallocate_list[(*current_list_size)++] = mext2_le_to_cpu32(block_numbers[i]);
            if(*current_list_size == max_block_list_size)
            {
                mext2_debug("List is full, deallocating");
                if(mext2_deallocate_blocks(sd, blocks_to_deallocate_list, max_block_list_size) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Couldn't dealocate blocks in list");
                    return MEXT2_RETURN_FAILURE;
                }
                *current_list_size = 0;

                if((block_numbers = (uint32_t*)mext2_get_ext2_block(sd, block_no)) == NULL)
                {
                    mext2_error("Could not read block %u", block_no);
                    return MEXT2_RETURN_FAILURE;
                }
            }
        }
    }


    if((block_numbers = (uint32_t*)mext2_get_ext2_block(sd, block_no)) == NULL)
    {
        mext2_error("Could not read block %u", block_no);
        return MEXT2_RETURN_FAILURE;
    }

    for(uint16_t i = (offset_fst_level_down == 0 ? offset : offset + 1); i < block_addresses_per_block && block_numbers[i] != 0; i++)
        block_numbers[i] = 0;

    if(mext2_put_ext2_block(sd, (block512_t*)block_numbers, block_no) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Couldn't save block %u", block_no);
        return MEXT2_RETURN_FAILURE;
    }

    if(offset == 0)
    {
        blocks_to_deallocate_list[(*current_list_size)++] = block_no;
        mext2_debug("Adding to list block %u", block_no);
        if(*current_list_size == max_block_list_size)
        {
            mext2_debug("List is full, deallocating");
            if(mext2_deallocate_blocks(sd, blocks_to_deallocate_list, max_block_list_size) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Couldn't dealocate blocks in list");
                return MEXT2_RETURN_FAILURE;
            }
            *current_list_size = 0;
        }
    }

    return MEXT2_RETURN_SUCCESS;
}

uint32_t mext2_get_data_block_by_inode_address_index(struct mext2_sd* sd, struct mext2_ext2_inode_address inode_address, uint32_t block_index)
{
    struct mext2_ext2_inode* inode;
    if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
    {
        return EXT2_INVALID_BLOCK_NO;
    }

    return mext2_get_data_block_by_inode_index(sd, inode, block_index);
}

uint32_t mext2_get_data_block_by_inode_index(struct mext2_sd* sd, struct mext2_ext2_inode* inode, uint32_t block_index)
{

    uint32_t block_addresses_per_block = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(uint32_t);
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

        if((addresses = (uint32_t*)mext2_get_ext2_block(sd, mext2_le_to_cpu32(inode->i_triple_indirect_block))) == NULL)
        {
            mext2_error("Can't read triple_indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        if((addresses = (uint32_t*)mext2_get_ext2_block(sd, mext2_le_to_cpu32(addresses[triple_indirect_index]))) == NULL)
        {
            mext2_error("Can't read double indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        if((addresses = (uint32_t*)mext2_get_ext2_block(sd, mext2_le_to_cpu32(addresses[double_indirect_index]))) == NULL)
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

        if((addresses = (uint32_t*)mext2_get_ext2_block(sd, mext2_le_to_cpu32(inode->i_double_indirect_block))) == NULL)
        {
            mext2_error("Can't read double block");
            return EXT2_INVALID_BLOCK_NO;
        }

        if((addresses = (uint32_t*)mext2_get_ext2_block(sd, mext2_le_to_cpu32(addresses[double_indirect_index]))) == NULL)
        {
            mext2_error("Can't read indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        mext2_debug("Block index: %u, Next data at block: %u", block_index, mext2_le_to_cpu32(addresses[indirect_index]));
        return mext2_le_to_cpu32(addresses[indirect_index]);
    }
    else
    {
        mext2_debug("Next data block in indirect block %u",inode->i_indirect_block);
        if((addresses = (uint32_t*)mext2_get_ext2_block(sd, inode->i_indirect_block)) == NULL)
        {
            mext2_error("Can't read indirect block");
            return EXT2_INVALID_BLOCK_NO;
        }

        mext2_debug("Block index: %u, Next data block at block: %u", block_index, mext2_le_to_cpu32(addresses[block_index - I_INDIRECT_BLOCK_INDEX]));
        return mext2_le_to_cpu32(addresses[block_index - I_INDIRECT_BLOCK_INDEX]);
    }

}

uint8_t mext2_ext2_inode_truncate(mext2_sd* sd, uint32_t inode_no, uint64_t new_size)
{
    const uint32_t last_block_index_to_leave = new_size / EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size);
    mext2_debug("Truncating, last block index to leave: %u", last_block_index_to_leave);

    struct mext2_ext2_inode_address inode_address;
    if((inode_address = mext2_get_ext2_inode_address(sd, inode_no)).block_address == EXT2_INVALID_BLOCK_NO)
    {
        mext2_error("Could not fetch inode %u at address", inode_no, inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    struct mext2_ext2_inode* inode;
    if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
    {
        mext2_error("Could not read inode %u", inode_no);
        return MEXT2_RETURN_FAILURE;
    }

    if((inode->i_mode & EXT2_FORMAT_MASK) != EXT2_S_IFREG)
    {
        mext2_error("Not a regular file, cannot truncate inode %u", inode_no);
        mext2_errno = MEXT2_DIR_INODE_TREATED_AS_REGULAR;
        return MEXT2_RETURN_FAILURE;
    }

    if(inode->i_size < new_size)
    {
        mext2_error("Cannot truncate to larger size, old size %u, new size: %u", inode->i_size, new_size);
        return MEXT2_RETURN_FAILURE;
    }

    uint32_t standard_block_no_list[MEXT2_MAX_BLOCK_NO_LIST_SIZE];
    uint32_t* block_list = &standard_block_no_list[0];
    uint16_t block_list_max_size = MEXT2_MAX_BLOCK_NO_LIST_SIZE;

    /* check if we have any additional space to store block numbers for deallocation */
    if(MEXT2_USEFULL_BLOCKS_SIZE * sizeof(block512_t) - EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) > 0)
    {
        block_list = (uint32_t*)(mext2_usefull_blocks + EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(block512_t));
        block_list_max_size = (MEXT2_USEFULL_BLOCKS_SIZE * sizeof(block512_t) - EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size)) / sizeof(block_list[0]);
    }

    uint16_t blocks_in_list = 0;
    uint16_t current_block_size = EXT2_INODE_BLOCK_SIZE(inode->i_blocks, sd->fs.descriptor.ext2.s_log_block_size);
    if(current_block_size == 0)
    {
        mext2_debug("No blocks in this inode, cannot truncate");
        return MEXT2_RETURN_SUCCESS;
    }
    if(current_block_size == last_block_index_to_leave + 1)
    {
        mext2_debug("Truncated 0 blocks");
    }

    uint32_t indirect_block_no = inode->i_indirect_block;
    uint32_t double_indirect_block_no = inode->i_double_indirect_block;
    uint32_t triple_indirect_block_no = inode->i_triple_indirect_block;

    const uint32_t block_addresses_per_block = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(uint32_t);
    const uint32_t first_block_index_in_double_indirect = I_INDIRECT_BLOCK_INDEX +  direct_block_addresses(block_addresses_per_block, INDIRECT_BLOCK);
    const uint32_t first_block_index_in_triple_indirect = first_block_index_in_double_indirect + direct_block_addresses(block_addresses_per_block, DOUBLE_INDIRECT_BLOCK);
    mext2_debug("block addresses per block: %u, first block index in double indirect: %u, first block index in triple indirect: %u", block_addresses_per_block, first_block_index_in_double_indirect, first_block_index_in_triple_indirect);

    if(last_block_index_to_leave >= first_block_index_in_triple_indirect)
    {
        if(free_blocks_with_list(sd, triple_indirect_block_no, last_block_index_to_leave - first_block_index_in_triple_indirect + 1, TRIPLE_INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
        {
            mext2_error("Could not read inode %u", inode_no);
            return MEXT2_RETURN_FAILURE;
        }
    }
    else if(last_block_index_to_leave >= first_block_index_in_double_indirect)
    {
        if(current_block_size > first_block_index_in_triple_indirect && free_blocks_with_list(sd, triple_indirect_block_no, 0, TRIPLE_INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if(free_blocks_with_list(sd, double_indirect_block_no, last_block_index_to_leave - first_block_index_in_double_indirect + 1, DOUBLE_INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
        {
            mext2_error("Could not read inode %u", inode_no);
            return MEXT2_RETURN_FAILURE;
        }
        inode->i_triple_indirect_block = 0;
    }
    else if(last_block_index_to_leave >= I_INDIRECT_BLOCK_INDEX)
    {
        if(current_block_size > first_block_index_in_triple_indirect && free_blocks_with_list(sd, triple_indirect_block_no, 0, TRIPLE_INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if(current_block_size > first_block_index_in_double_indirect && free_blocks_with_list(sd, double_indirect_block_no, 0, DOUBLE_INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if(free_blocks_with_list(sd, indirect_block_no, last_block_index_to_leave - I_INDIRECT_BLOCK_INDEX + 1, INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
        {
            mext2_error("Could not read inode %u", inode_no);
            return MEXT2_RETURN_FAILURE;
        }

        inode->i_triple_indirect_block = 0;
        inode->i_double_indirect_block = 0;
    }
    else
    {
        if(current_block_size > first_block_index_in_triple_indirect && free_blocks_with_list(sd, triple_indirect_block_no, 0, TRIPLE_INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if(current_block_size > first_block_index_in_double_indirect && free_blocks_with_list(sd, double_indirect_block_no, 0, DOUBLE_INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if(current_block_size > I_INDIRECT_BLOCK_INDEX && free_blocks_with_list(sd, indirect_block_no, 0, INDIRECT_BLOCK, block_list, block_list_max_size, &blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocate error");
            return MEXT2_RETURN_FAILURE;
        }

        if((inode = mext2_get_ext2_inode_by_no(sd, inode_no)) == NULL)
        {
            mext2_error("Could not read inode %u", inode_no);
            return MEXT2_RETURN_FAILURE;
        }

        for(uint8_t i = last_block_index_to_leave + 1; i < current_block_size && i < I_INDIRECT_BLOCK_INDEX; i++)
        {
            block_list[blocks_in_list++] = inode->i_direct_block[i];
            mext2_debug("Adding to list direct block no %u", inode->i_direct_block[i]);
            if(blocks_in_list == block_list_max_size)
            {
                mext2_debug("list is full, deallocating");
                if(mext2_deallocate_blocks(sd, block_list, blocks_in_list) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Deallocation failed, truncating file impossible");
                    return MEXT2_RETURN_FAILURE;
                }
                blocks_in_list = 0;

                if((inode = mext2_get_ext2_inode_by_no(sd, inode_no)) == NULL)
                {
                    mext2_error("Could not read inode %u", inode_no);
                    return MEXT2_RETURN_FAILURE;
                }
            }
        }

        for(uint8_t i = last_block_index_to_leave + 1; i < current_block_size && i < I_INDIRECT_BLOCK_INDEX; i++)
        {
            inode->i_direct_block[i] = 0;
        }

        inode->i_triple_indirect_block = 0;
        inode->i_double_indirect_block = 0;
        inode->i_indirect_block = 0;
    }

    inode->i_size = (uint32_t)new_size;
    if(sd->fs.descriptor.ext2.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE)
        inode->i_dir_acl = (uint32_t)(new_size >> (sizeof(uint32_t) * BITS_IN_BYTE));

    inode->i_blocks = (last_block_index_to_leave + 1) * (EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(block512_t));

    if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not write inode %u at address %u", inode_no , inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    if(blocks_in_list != 0)
    {
        mext2_debug("Blocks in list: %u, deallocating", blocks_in_list);
        if(mext2_deallocate_blocks(sd, block_list, blocks_in_list) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Deallocation failed, truncating file impossible");
            return MEXT2_RETURN_FAILURE;
        }
        blocks_in_list = 0;
    }

    mext2_log("Truncated %u blocks", current_block_size - (last_block_index_to_leave + 1));
    return MEXT2_RETURN_SUCCESS;
}

struct mext2_ext2_superblock* mext2_get_main_ext2_superblock(struct mext2_sd* sd)
{
    if(mext2_read_blocks(sd, sd->partition_block_addr + EXT2_SUPERBLOCK_PARTITION_BLOCK_OFFSET, mext2_usefull_blocks, EXT2_SUPERBLOCK_PHYSICAL_BLOCK_SIZE) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not read superblock");
        mext2_errno = MEXT2_READ_ERROR;
        return NULL;
    }

    return (struct mext2_ext2_superblock*)&mext2_usefull_blocks[0];
}

STATIC uint32_t allocate_ext2_inode_one_group(mext2_sd* sd, uint16_t block_group_no)
{
    const uint32_t ext2_block_size = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size);
    block512_t* block;
    struct mext2_ext2_block_group_descriptor* block_group_descriptor;
    if((block = mext2_get_ext2_block(
            sd,
            BGD_BLOCK_NO(block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)))
        == NULL)
    {
        mext2_error("Could not read block group descriptor");
        return 0;
    }

    block_group_descriptor = (struct mext2_ext2_block_group_descriptor*)block;
    block_group_descriptor += BGD_BLOCK_OFFSET(block_group_no, ext2_block_size);

    uint32_t inode_bitmap_block = mext2_le_to_cpu32(block_group_descriptor->bg_inode_bitmap);
    uint8_t* inode_bitmap;

    uint32_t first_inode_in_block_group = block_group_no * sd->fs.descriptor.ext2.s_inodes_per_group + 1;

    if((inode_bitmap = (uint8_t*)mext2_get_ext2_block(sd, inode_bitmap_block)) == NULL)
    {
        mext2_error("Can't read inode bitmap at block %u", inode_bitmap_block);
        return EXT2_INVALID_INO;
    }

    mext2_debug("Read inode bitmap at block %u", inode_bitmap_block);
    for(int i = 0; i < ext2_block_size * BITS_IN_BYTE; i++)
    {
        if(!(inode_bitmap[i / BITS_IN_BYTE] & (1 << (i % BITS_IN_BYTE))))
        {
            inode_bitmap[i / BITS_IN_BYTE] |= (1 << (i % BITS_IN_BYTE));
            if(mext2_put_ext2_block(sd, (block512_t*)inode_bitmap, inode_bitmap_block) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Could not update inode bitmap at block %u", inode_bitmap_block);
                return MEXT2_RETURN_FAILURE;
            }

            if((block = mext2_get_ext2_block(
                    sd,
                    BGD_BLOCK_NO(block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)))
                == NULL)
            {
                mext2_error("Could not read block group descriptor");
                return 0;
            }

            block_group_descriptor = (struct mext2_ext2_block_group_descriptor*)block;
            block_group_descriptor += BGD_BLOCK_OFFSET(block_group_no, ext2_block_size);

            block_group_descriptor->bg_free_inodes_count = mext2_cpu_to_le16(mext2_le_to_cpu16(block_group_descriptor->bg_free_inodes_count) - 1);
            if(mext2_put_ext2_block(sd, block,
                    BGD_BLOCK_NO(block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block)) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Could not update block group descriptor no %u, at block %u", block_group_no,
                        BGD_BLOCK_NO(block_group_no, ext2_block_size, sd->fs.descriptor.ext2.s_first_data_block));
                return EXT2_INVALID_INO;
            }

            mext2_debug("Succesfully allocated inode %u", first_inode_in_block_group + i);
            return first_inode_in_block_group + i;
        }
    }

    mext2_errno = MEXT2_INODE_BITMAP_FULL;
    return EXT2_INVALID_INO;
}

STATIC uint32_t allocate_ext2_inode(mext2_sd* sd, uint16_t primary_block_group_no)
{
    uint32_t inode_no;
    if((inode_no = allocate_ext2_inode_one_group(sd, primary_block_group_no)) == EXT2_INVALID_INO)
    {
        uint16_t block_groups_count = (sd->fs.descriptor.ext2.s_blocks_count + sd->fs.descriptor.ext2.s_blocks_per_group - 1) / sd->fs.descriptor.ext2.s_blocks_per_group;
        for(uint16_t i = 0; i < block_groups_count && mext2_errno == MEXT2_INODE_BITMAP_FULL; i++)
        {
            if(i == primary_block_group_no)
                continue;

            if((inode_no = allocate_ext2_inode_one_group(sd, i)) != EXT2_INVALID_INO)
                break;
        }
    }

    if(inode_no == EXT2_INVALID_INO)
    {
        mext2_error("Could not allocate new inode");
        if(mext2_errno == MEXT2_INODE_BITMAP_FULL)
            mext2_errno = MEXT2_FS_FULL;
    }

    return inode_no;
}

uint8_t mext2_update_ext2_main_superblock_with_ptr(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock)
{
    mext2_debug("Superblock magic: %#hx", mext2_le_to_cpu16(superblock->s_magic));
    if(mext2_write_blocks(sd, sd->partition_block_addr + EXT2_SUPERBLOCK_PARTITION_BLOCK_OFFSET, (block512_t*)superblock, EXT2_SUPERBLOCK_PHYSICAL_BLOCK_SIZE) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not update main superblock");
        mext2_errno = MEXT2_WRITE_ERROR;
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}

uint8_t mext2_update_ext2_main_superblock(struct mext2_sd* sd)
{
    struct mext2_ext2_superblock* superblock;
    if((superblock = mext2_get_main_ext2_superblock(sd)) == NULL)
        return MEXT2_RETURN_FAILURE;

    mext2_debug("Free blocks count %u, Free inodes count %u", sd->fs.descriptor.ext2.s_free_blocks_count, sd->fs.descriptor.ext2.s_free_inodes_count);

    if(mext2_is_big_endian())
    {
        superblock->s_free_blocks_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_free_blocks_count);
        superblock->s_free_inodes_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_free_inodes_count);
    }
    else
    {
        superblock->s_free_blocks_count = sd->fs.descriptor.ext2.s_free_blocks_count;
        superblock->s_free_inodes_count = sd->fs.descriptor.ext2.s_free_inodes_count;
    }

    if(mext2_update_ext2_main_superblock_with_ptr(sd, superblock) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not update main superblock");
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t update_superblock_group_no(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock, uint16_t block_group_no)
{
    if(block_group_no == 0)
    {
        mext2_warning("Trying to update superblock at group 0 with update_superblock_group_no, use mext2_update_ext2_main_superblock instead");
        superblock->s_block_group_nr = 0;
        mext2_update_ext2_main_superblock_with_ptr(sd, superblock);
    }

    uint32_t superblock_block_no = block_group_no * sd->fs.descriptor.ext2.s_blocks_per_group;
    superblock->s_block_group_nr = mext2_cpu_to_le16(block_group_no);

    if(mext2_write_blocks(sd, sd->partition_block_addr + superblock_block_no * (EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(block512_t)), (block512_t*)superblock, EXT2_SUPERBLOCK_PHYSICAL_BLOCK_SIZE) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not update superblock no %u", block_group_no);
        mext2_errno = MEXT2_WRITE_ERROR;
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}

uint8_t mext2_update_ext2_superblocks_with_ptr(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock)
{
    if(mext2_update_ext2_main_superblock_with_ptr(sd, superblock) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not update main superblock");
        return MEXT2_RETURN_FAILURE;
    }

    if((superblock = mext2_get_main_ext2_superblock(sd)) == NULL)
        return MEXT2_RETURN_FAILURE;

    uint16_t block_groups_count = (superblock->s_blocks_count + superblock->s_blocks_per_group - 1) / superblock->s_blocks_per_group;
    mext2_debug("Blocks count: %u, Blocks per group: %u, Block groups count %hu", superblock->s_blocks_count, superblock->s_blocks_per_group, block_groups_count);
    if(sd->fs.descriptor.ext2.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)
    {
        if(block_groups_count > 1 && update_superblock_group_no(sd, superblock, 1) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Could not update superblock in block group %u", 1);
            return MEXT2_RETURN_FAILURE;
        }

        if((superblock = mext2_get_main_ext2_superblock(sd)) == NULL)
            return MEXT2_RETURN_FAILURE;

        if(block_groups_count > 3)
            for(uint16_t block_group = 3; block_group < block_groups_count; block_group *= 3)
            {
                if((superblock = mext2_get_main_ext2_superblock(sd)) == NULL)
                    return MEXT2_RETURN_FAILURE;
                if(update_superblock_group_no(sd, superblock, block_group) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update superblock in block group %u", block_group);
                    return MEXT2_RETURN_FAILURE;
                }
            }

        if(block_groups_count > 5)
            for(uint16_t block_group = 5; block_group < block_groups_count; block_group *= 5)
            {
                if((superblock = mext2_get_main_ext2_superblock(sd)) == NULL)
                    return MEXT2_RETURN_FAILURE;
                if(update_superblock_group_no(sd, superblock, block_group) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update superblock in block group %u", block_group);
                    return MEXT2_RETURN_FAILURE;
                }
            }

        if(block_groups_count > 7)
            for(uint16_t block_group = 7; block_group < block_groups_count; block_group *= 7)
            {
                if((superblock = mext2_get_main_ext2_superblock(sd)) == NULL)
                    return MEXT2_RETURN_FAILURE;
                if(update_superblock_group_no(sd, superblock, block_group) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update superblock in block group %u", block_group);
                    return MEXT2_RETURN_FAILURE;
                }
            }
    }
    else
    {
        for(uint16_t block_group = 1; block_group < block_groups_count; block_group++)
        {
            if(update_superblock_group_no(sd, superblock, block_group) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Could not update superblock in block group %u", block_group);
                return MEXT2_RETURN_FAILURE;
            }

            if((superblock = mext2_get_main_ext2_superblock(sd)) == NULL)
                return MEXT2_RETURN_FAILURE;
        }
    }

    return MEXT2_RETURN_SUCCESS;
}

uint8_t mext2_update_ext2_superblocks(struct mext2_sd* sd)
{

    struct mext2_ext2_superblock* superblock;
    if((superblock = mext2_get_main_ext2_superblock(sd)) == NULL)
        return MEXT2_RETURN_FAILURE;

    mext2_debug("Superblock magic: %#hx", mext2_le_to_cpu16(superblock->s_magic));

    if(mext2_is_big_endian())
    {
        superblock->s_free_blocks_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_free_blocks_count);
        superblock->s_free_inodes_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_free_inodes_count);
    }
    else
    {
        superblock->s_free_blocks_count = sd->fs.descriptor.ext2.s_free_blocks_count;
        superblock->s_free_inodes_count = sd->fs.descriptor.ext2.s_free_inodes_count;
    }

    return mext2_update_ext2_superblocks_with_ptr(sd, superblock);
}

uint8_t mext2_update_ext2_block_group_descriptors(struct mext2_sd* sd)
{
    const uint32_t ext2_block_size = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size);
    const uint16_t block_groups_count = (sd->fs.descriptor.ext2.s_blocks_count + sd->fs.descriptor.ext2.s_blocks_per_group - 1) / sd->fs.descriptor.ext2.s_blocks_per_group;

    const uint8_t block_group_descriptors_block_size = (block_groups_count * sizeof(struct mext2_ext2_block_group_descriptor) + ext2_block_size - 1) / ext2_block_size;

    block512_t* block;
    for(uint8_t i = 0; i < block_group_descriptors_block_size; i++)
    {
        if((block = mext2_get_ext2_block(sd, sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i)) == NULL)
        {
            mext2_error("Could not read block group descriptors block", sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
            return MEXT2_RETURN_FAILURE;
        }

        if(sd->fs.descriptor.ext2.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)
        {
            if(block_groups_count > 1 && mext2_put_ext2_block(sd, block, sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Cannot update block group descriptor in group %u at block %u", 1, sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                return MEXT2_RETURN_FAILURE;
            }

            if(block_groups_count > 3)
                for(uint16_t block_group = 3; block_group < block_groups_count; block_group *= 3)
                {
                    if((block = mext2_get_ext2_block(sd, sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i)) == NULL)
                    {
                        mext2_error("Could not read block group descriptors block", sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                        return MEXT2_RETURN_FAILURE;
                    }

                    if(mext2_put_ext2_block(sd, block, block_group * sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Cannot update block group descriptor in group %u at block %u", block_group, block_group * sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                        return MEXT2_RETURN_FAILURE;
                    }
                }

            if(block_groups_count > 5)
                for(uint16_t block_group = 5; block_group < block_groups_count; block_group *= 5)
                {
                    if((block = mext2_get_ext2_block(sd, sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i)) == NULL)
                    {
                        mext2_error("Could not read block group descriptors block", sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                        return MEXT2_RETURN_FAILURE;
                    }

                    if(mext2_put_ext2_block(sd, block, block_group * sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Cannot update block group descriptor in group %u at block %u", block_group, block_group * sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                        return MEXT2_RETURN_FAILURE;
                    }
                }

            if(block_groups_count > 7)
                for(uint16_t block_group = 7; block_group < block_groups_count; block_group *= 7)
                {
                    if((block = mext2_get_ext2_block(sd, sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i)) == NULL)
                    {
                        mext2_error("Could not read block group descriptors block", sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                        return MEXT2_RETURN_FAILURE;
                    }

                    if(mext2_put_ext2_block(sd, block, block_group * sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Cannot update block group descriptor in group %u at block %u", block_group, block_group * sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                        return MEXT2_RETURN_FAILURE;
                    }
                }
        }
        else
        {
            for(uint16_t block_group = 1; block_group < block_groups_count; block_group++)
            {
                if((block = mext2_get_ext2_block(sd, sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i)) == NULL)
                {
                    mext2_error("Could not read block group descriptors block", sd->fs.descriptor.ext2.s_first_data_block + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                    return MEXT2_RETURN_FAILURE;
                }

                if(mext2_put_ext2_block(sd, block, block_group * sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Cannot update block group descriptor in group %u at block %u", block_group, block_group * sd->fs.descriptor.ext2.s_blocks_per_group + EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET + i);
                    return MEXT2_RETURN_FAILURE;
                }
            }
        }
    }

    return MEXT2_RETURN_SUCCESS;
}

uint8_t mext2_put_ext2_block(struct mext2_sd* sd, block512_t* block, uint32_t block_no)
{
    const uint16_t ext2_block_block_size = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(block512_t);
    uint32_t block_block_address = sd->partition_block_addr + block_no * ext2_block_block_size;

    if(block_no >= sd->fs.descriptor.ext2.s_blocks_count)
    {
        mext2_error("Attempt to read block %u, while last block in fs is %u", block_no, sd->fs.descriptor.ext2.s_blocks_count);
        mext2_errno = MEXT2_INVALID_BLOCK_NO;
        return MEXT2_RETURN_FAILURE;
    }

    if(mext2_write_blocks(sd, block_block_address, block, ext2_block_block_size) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Cannot write block");
        mext2_errno = MEXT2_WRITE_ERROR;
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}

block512_t* mext2_get_ext2_block(struct mext2_sd* sd, uint32_t block_no)
{
    const uint16_t ext2_block_block_size = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(block512_t);
    uint32_t block_block_address = sd->partition_block_addr + block_no * ext2_block_block_size;

    if(block_no >= sd->fs.descriptor.ext2.s_blocks_count)
    {
        mext2_error("Attempt to read block %u, while last block in fs is %u", block_no, sd->fs.descriptor.ext2.s_blocks_count);
        mext2_errno = MEXT2_INVALID_BLOCK_NO;
        return NULL;
    }

    if(mext2_read_blocks(sd, block_block_address, mext2_usefull_blocks, ext2_block_block_size) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Can't read block no %d, at address %#x", block_no, block_block_address);
        mext2_errno = MEXT2_READ_ERROR;
        return NULL;
    }

    return &mext2_usefull_blocks[0];
}


uint8_t mext2_put_ext2_inode(struct mext2_sd* sd, struct mext2_ext2_inode* inode, struct mext2_ext2_inode_address address)
{
    if(mext2_is_big_endian())
        flip_inode_endianess(inode);
    /* TODO Checking if inode is in usefull blocks, if so read the block and copy inode */
    if(mext2_put_ext2_block(sd, mext2_usefull_blocks, address.block_address) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Can't write inode at address %u", address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    if(mext2_is_big_endian())
        flip_inode_endianess(inode);

    return MEXT2_RETURN_SUCCESS;
}

struct mext2_ext2_inode_address mext2_get_ext2_inode_address(struct mext2_sd* sd, uint32_t inode_no)
{
    struct mext2_ext2_inode_address address;
    uint32_t block_group_descriptor_block_no = ((inode_no - 1) / sd->fs.descriptor.ext2.s_inodes_per_group)
            * sizeof(struct mext2_ext2_block_group_descriptor) / EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size)
            + BLOCK_GROUP_DESCRIPTOR_TABLE_FIRST_BLOCK_OFFSET + sd->fs.descriptor.ext2.s_first_data_block;

    uint32_t block_group_in_block_offset = (((inode_no - 1) / sd->fs.descriptor.ext2.s_inodes_per_group) // block_group number
            * sizeof(struct mext2_ext2_block_group_descriptor)) % EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size); // find offset in block

    block512_t* block;
    if((block = mext2_get_ext2_block(sd, block_group_descriptor_block_no)) == NULL)
    {
        mext2_error("Could not read block group descriptor", block_group_descriptor_block_no);
        address.block_address = EXT2_INVALID_BLOCK_NO;
        return address;
    }
    struct mext2_ext2_block_group_descriptor* bgd = (struct mext2_ext2_block_group_descriptor*) (((uint8_t*)block) + block_group_in_block_offset);

    if(mext2_is_big_endian())
        flip_block_group_descriptor_endianess(bgd);

    uint32_t inode_local_index = (inode_no - 1) % sd->fs.descriptor.ext2.s_inodes_per_group;

    address.block_address = (bgd->bg_inode_table + ((inode_local_index
            * sd->fs.descriptor.ext2.s_inode_size)
            / EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size)));
    address.block_offset = ((inode_local_index * sd->fs.descriptor.ext2.s_inode_size) % EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));

    return address;
}

struct mext2_ext2_inode* mext2_get_ext2_inode(struct mext2_sd* sd, struct mext2_ext2_inode_address address)
{
    block512_t* block;
    if((block = mext2_get_ext2_block(sd, address.block_address)) == NULL)
    {
        mext2_error("Can't read inode at block %u", address.block_address);
        return NULL;
    }
    struct mext2_ext2_inode* inode = (struct mext2_ext2_inode*)(((uint8_t*)block) + address.block_offset);

    if(mext2_is_big_endian())
        flip_inode_endianess(inode);

    return inode;
}

struct mext2_ext2_inode* mext2_get_ext2_inode_by_no(struct mext2_sd* sd, uint32_t inode_no)
{
    struct mext2_ext2_inode_address address;
    if((address = mext2_get_ext2_inode_address(sd, inode_no)).block_address == EXT2_INVALID_BLOCK_NO)
    {
        mext2_error("Can't read block address containing inode");
        return NULL;
    }

    return mext2_get_ext2_inode(sd, address);
}

STATIC void correct_dir_entry_head_endianess(struct mext2_ext2_dir_entry_head* head)
{
    head->inode = mext2_flip_endianess32(head->inode);
    head->rec_len = mext2_flip_endianess16(head->rec_len);
}

STATIC uint32_t find_inode_no_direct_block(mext2_sd* sd, uint32_t direct_block_address, char* name)
{

    mext2_debug("Looking for name '%s' in direct block at address %#x", name, direct_block_address);
    block512_t* block;
    if((block = mext2_get_ext2_block(sd, direct_block_address)) == NULL)
    {
        mext2_error("Could not read direct block");
        return EXT2_INVALID_INO;
    }
    struct mext2_ext2_dir_entry* dir_entry = (struct mext2_ext2_dir_entry*)block;
    uint16_t pos = 0;

    while(pos < EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size))
    {
        if(mext2_is_big_endian())
            correct_dir_entry_head_endianess(&dir_entry->head);
        mext2_debug("Inode: %d, Record length: %d, Name length: %d, File type: %d, Name: %.*s",
            dir_entry->head.inode,
            dir_entry->head.rec_len,
            dir_entry->head.name_len,
            dir_entry->head.file_type,
            dir_entry->head.name_len,
            dir_entry->name
        );
        if(dir_entry->head.inode != EXT2_INVALID_INO && strncmp(name, &dir_entry->name[0], strlen(&name[0])) == 0)
        {
            mext2_log("Inode number %d found for: '%s'", dir_entry->head.inode, name);
            return dir_entry->head.inode;
        }
        pos += dir_entry->head.rec_len;
        dir_entry = (struct mext2_ext2_dir_entry*)(((uint8_t*)block) + pos);
    }

    mext2_errno = MEXT2_INVALID_PATH;
    return EXT2_INVALID_INO;
}

STATIC uint32_t find_inode_no_indirect_block(mext2_sd* sd, uint32_t indirect_block_address, char* name)
{
    mext2_debug("Looking for name '%s' in indirect block at address %#x", name, indirect_block_address);
    uint32_t* direct_blocks;
    uint32_t inode_no;

    uint32_t direct_block_index = 0;
    for(direct_block_index = 0; direct_block_index < EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(uint32_t); direct_block_index++)
    {
        if((direct_blocks = (uint32_t*)mext2_get_ext2_block(sd, indirect_block_address)) == NULL)
        {
            mext2_error("Could not read indirect block");
            return EXT2_INVALID_INO;
        }
        if(direct_blocks[direct_block_index] == 0)
        {
            mext2_errno = MEXT2_INVALID_PATH;
            return EXT2_INVALID_INO;
        }

        mext2_errno = MEXT2_NO_ERRORS;
        if((inode_no = find_inode_no_direct_block(sd, mext2_le_to_cpu32(direct_blocks[direct_block_index]), name)) != EXT2_INVALID_INO)
            return inode_no;

        if(mext2_errno != MEXT2_INVALID_PATH)
            return EXT2_INVALID_INO;

    }

    mext2_errno = MEXT2_INVALID_PATH;
    return EXT2_INVALID_INO;
}

STATIC uint32_t find_inode_no_double_indirect_block(mext2_sd* sd, uint32_t double_indirect_block_address, char* name)
{
    mext2_debug("Looking for name '%s' in double indirect block at address %#x", name, double_indirect_block_address);
    uint32_t* indirect_blocks;
    uint32_t inode_no;

    uint32_t indirect_block_index = 0;
    for(indirect_block_index = 0; indirect_block_index < EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(uint32_t); indirect_block_index++)
    {
        if((indirect_blocks = (uint32_t*)mext2_get_ext2_block(sd, double_indirect_block_address)) == NULL)
        {
            mext2_error("Could not read double block");
            return EXT2_INVALID_INO;
        }
        if(indirect_blocks[indirect_block_index] == 0)
        {
            mext2_error("Could not find inode number for specified path");
            mext2_errno = MEXT2_INVALID_PATH;
            return EXT2_INVALID_INO;
        }

        mext2_errno = MEXT2_NO_ERRORS;
        if((inode_no = find_inode_no_indirect_block(sd, mext2_le_to_cpu32(indirect_blocks[indirect_block_index]), name)) != EXT2_INVALID_INO)
            return inode_no;

        if(mext2_errno != MEXT2_INVALID_PATH)
            return EXT2_INVALID_INO;
    }

    mext2_errno = MEXT2_INVALID_PATH;
    return EXT2_INVALID_INO;
}

STATIC uint32_t find_inode_no_triple_indirect_block(mext2_sd* sd, uint32_t triple_indirect_block_address, char* name)
{
    mext2_debug("Looking for name '%s' in triple indirect block at address %#x", name, triple_indirect_block_address);
    uint32_t* double_indirect_blocks;
    uint32_t inode_no;

    uint32_t double_indirect_block_index = 0;
    for(double_indirect_block_index = 0; double_indirect_block_index < EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(uint32_t); double_indirect_block_index++)
    {
        if((double_indirect_blocks = (uint32_t*)mext2_get_ext2_block(sd, triple_indirect_block_address)) == NULL)
        {
            mext2_error("Could not read triple indirect block");
            return EXT2_INVALID_INO;
        }
        if(double_indirect_blocks[double_indirect_block_index] == 0)
        {
            mext2_error("Could not find inode number for specified path");
            mext2_errno = MEXT2_INVALID_PATH;
            return EXT2_INVALID_INO;
        }

        mext2_errno = MEXT2_NO_ERRORS;
        if((inode_no = find_inode_no_double_indirect_block(sd, mext2_le_to_cpu32(double_indirect_blocks[double_indirect_block_index]), name)) != EXT2_INVALID_INO)
            return inode_no;

        if(mext2_errno != MEXT2_INVALID_PATH)
            return EXT2_INVALID_INO;
    }

    mext2_error("Could not find inode number for specified path");
    mext2_errno = MEXT2_INVALID_PATH;
    return EXT2_INVALID_INO;
}

uint32_t mext2_inode_no_lookup_from_dir_inode(struct mext2_sd* sd, uint32_t dir_inode_no, char* name)
{
    struct mext2_ext2_inode* inode;
    if((inode = mext2_get_ext2_inode_by_no(sd, dir_inode_no)) == NULL)
    {
        mext2_error("Could not read inode %d", dir_inode_no);
        return EXT2_INVALID_INO;
    }

    if((inode->i_mode & EXT2_FORMAT_MASK) != EXT2_S_IFDIR)
    {
        mext2_error("Inode no: %d is not a directory inode", dir_inode_no);
        mext2_errno = MEXT2_INVALID_PATH;
        return EXT2_INVALID_INO;
    }

    mext2_debug("Inode mode: %#hx", inode->i_mode);
    mext2_debug("Inode i_size: %#x", inode->i_size);
    mext2_debug("Inode i_blocks: %d", inode->i_blocks);


    uint32_t blocks[15];
    uint32_t used_blocks = EXT2_INODE_BLOCK_SIZE(inode->i_blocks, sd->fs.descriptor.ext2.s_log_block_size);

    memcpy(blocks, &inode->i_direct_block, sizeof(inode->i_direct_block) / sizeof(inode->i_direct_block[0]));
    blocks[I_INDIRECT_BLOCK_INDEX] = inode->i_indirect_block;
    blocks[I_DOUBLE_INDIRECT_BLOCK_INDEX] = inode->i_double_indirect_block;
    blocks[I_TRIPLE_INDIRECT_BLOCK_INDEX] = inode->i_triple_indirect_block;
    uint8_t direct_blocks = sizeof(inode->i_direct_block) / sizeof(inode->i_direct_block[0]) > used_blocks ?
            used_blocks : sizeof(inode->i_direct_block) / sizeof(inode->i_direct_block[0]);

    uint32_t inode_no;
    for(uint8_t i = 0; i < direct_blocks; i++)
    {
        if((inode_no = find_inode_no_direct_block(sd, blocks[i], name)) == EXT2_INVALID_INO)
        {
            if(mext2_errno != MEXT2_INVALID_PATH)
                return EXT2_INVALID_INO;
        }
        else
        {
            return inode_no;
        }
    }

    if(blocks[I_INDIRECT_BLOCK_INDEX] != 0)
    {
        if((inode_no = find_inode_no_indirect_block(sd, blocks[I_INDIRECT_BLOCK_INDEX], name)) == EXT2_INVALID_INO)
        {
            if(mext2_errno != MEXT2_INVALID_PATH)
                return EXT2_INVALID_INO;
        }
        else
        {
            return inode_no;
        }

        if(blocks[I_DOUBLE_INDIRECT_BLOCK_INDEX] != 0)
        {
            if((inode_no = find_inode_no_double_indirect_block(sd, blocks[I_DOUBLE_INDIRECT_BLOCK_INDEX], name)) == EXT2_INVALID_INO)
            {
                if(mext2_errno != MEXT2_INVALID_PATH)
                    return EXT2_INVALID_INO;
            }
            else
            {
                return inode_no;
            }

            if(blocks[I_TRIPLE_INDIRECT_BLOCK_INDEX] != 0)
            {
                if((inode_no = find_inode_no_triple_indirect_block(sd, blocks[I_TRIPLE_INDIRECT_BLOCK_INDEX], name)) == EXT2_INVALID_INO)
                {
                    if(mext2_errno != MEXT2_INVALID_PATH)
                        return EXT2_INVALID_INO;
                }
                else
                {
                    return inode_no;
                }
            }
        }
    }

    return EXT2_INVALID_INO;
}

STATIC char* path_tokenize(char* path)
{
    static char* token_beggining = NULL;
    if(path != NULL)
    {
        token_beggining = path;
        while(*token_beggining == MEXT2_PATH_SEPARATOR)
            token_beggining++;

        if(*token_beggining != '\0')
            return token_beggining;
        else
        {
            token_beggining = NULL;
            return NULL;
        }
    }

    if(token_beggining == NULL)
        return NULL;

    token_beggining = strchr(token_beggining, MEXT2_PATH_SEPARATOR);

    if(token_beggining == NULL)
        return NULL;

    while(*token_beggining  == MEXT2_PATH_SEPARATOR)
        token_beggining++;
    if(*token_beggining != '\0')
        return token_beggining;
    else
        return NULL;
}

STATIC uint8_t mext2_inode_address_insert_new_blocks(struct mext2_sd* sd, struct mext2_ext2_inode_address inode_address, uint32_t* block_list, uint16_t block_list_length)
{
    struct mext2_ext2_inode* inode;
    if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
    {
        mext2_error("Could not read inode at address %u in order to insert new blocks", inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    uint32_t ext2_blocks_count = EXT2_INODE_BLOCK_SIZE(inode->i_blocks, sd->fs.descriptor.ext2.s_log_block_size);
    uint32_t block_addresses_per_block = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(uint32_t);
    uint32_t first_block_index_in_double_indirect = I_INDIRECT_BLOCK_INDEX + block_addresses_per_block;
    uint32_t first_block_index_in_triple_indirect = first_block_index_in_double_indirect + block_addresses_per_block * block_addresses_per_block;
    uint32_t maximum_inode_blocks = first_block_index_in_triple_indirect + block_addresses_per_block * block_addresses_per_block * block_addresses_per_block;
    uint16_t inserted_blocks = 0;
    while(inserted_blocks < block_list_length && ext2_blocks_count < maximum_inode_blocks)
    {
        if(ext2_blocks_count < I_INDIRECT_BLOCK_INDEX)
        {
            for(; ext2_blocks_count < I_INDIRECT_BLOCK_INDEX && inserted_blocks < block_list_length; ext2_blocks_count++)
                inode->i_direct_block[ext2_blocks_count] = block_list[inserted_blocks++];

            if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Could not update inode at address %u during process of putting new blocks in direct block fields", inode_address.block_address);
                return MEXT2_RETURN_FAILURE;
            }
        }
        else if(ext2_blocks_count < first_block_index_in_double_indirect)
        {
            uint32_t indirect_block_no;
            uint32_t* indirect_block;
            if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
            {
                mext2_error("Could not read inode at address %u in order to insert new blocks", inode_address.block_address);
                return MEXT2_RETURN_FAILURE;
            }

            if((indirect_block_no = inode->i_indirect_block) == EXT2_INVALID_INO)
            {
                if(mext2_allocate_blocks(sd, inode_address.block_address / sd->fs.descriptor.ext2.s_blocks_per_group, &indirect_block_no, 1) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not allocate indirect block for inode at address %u", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                if((indirect_block = (uint32_t*)mext2_get_ext2_block(sd, indirect_block_no)) == NULL)
                {
                    mext2_error("Could not fetch newly allocated indirect block %u for clearing", indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                memset((void*)indirect_block, 0, EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));
                if(mext2_put_ext2_block(sd, (block512_t*)indirect_block, indirect_block_no) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update newly allocated indirect block %u after clearing", indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
                {
                    mext2_error("Could not read inode at address %u in order to insert new indirect block", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                inode->i_indirect_block = indirect_block_no;
                if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update inode at address %u after allocating new indirect block", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                mext2_debug("Allocated new indirect block - %u", indirect_block_no);
            }

            if((indirect_block = (uint32_t*)mext2_get_ext2_block(sd, indirect_block_no)) == NULL)
            {
                mext2_error("Could not read indirect block %u", indirect_block_no);
                return MEXT2_RETURN_FAILURE;
            }

            for(uint32_t block_index = ext2_blocks_count - I_INDIRECT_BLOCK_INDEX; ext2_blocks_count < first_block_index_in_double_indirect
                                                                                    && inserted_blocks < block_list_length; block_index++)
            {
                indirect_block[block_index] = mext2_cpu_to_le32(block_list[inserted_blocks++]);
                mext2_debug("Inserted new block %u at index %u", indirect_block[block_index], ext2_blocks_count);
                ext2_blocks_count++;
            }

            if(mext2_put_ext2_block(sd, (block512_t*)indirect_block, indirect_block_no) != MEXT2_RETURN_SUCCESS)
            {
                mext2_error("Could not update indirect block %u after clearing", indirect_block_no);
                return MEXT2_RETURN_FAILURE;
            }
        }
        else if(ext2_blocks_count < first_block_index_in_triple_indirect)
        {
            uint32_t double_indirect_block_no;
            uint32_t* double_indirect_block;
            if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
            {
                mext2_error("Could not read inode at address %u in order to insert new blocks", inode_address.block_address);
                return MEXT2_RETURN_FAILURE;
            }

            if((double_indirect_block_no = inode->i_double_indirect_block) == EXT2_INVALID_INO)
            {
                if(mext2_allocate_blocks(sd, inode_address.block_address / sd->fs.descriptor.ext2.s_blocks_per_group, &double_indirect_block_no, 1) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not allocate double indirect block for inode at address %u", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                if((double_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, double_indirect_block_no)) == NULL)
                {
                    mext2_error("Could not fetch newly allocated double indirect block %u for clearing", double_indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                memset((void*)double_indirect_block, 0, EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));
                if(mext2_put_ext2_block(sd, (block512_t*)double_indirect_block, double_indirect_block_no) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update newly allocated double indirect block %u after clearing", double_indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
                {
                    mext2_error("Could not read inode at address %u in order to insert new double indirect", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                inode->i_double_indirect_block = double_indirect_block_no;
                if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update inode at address %u after allocating new double indirect block", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                mext2_debug("Allocated new double indirect block - %u", double_indirect_block_no);
            }

            for(uint32_t indirect_block_index = (ext2_blocks_count - first_block_index_in_double_indirect) / block_addresses_per_block;
                                                 ext2_blocks_count < first_block_index_in_triple_indirect
                                                 && inserted_blocks < block_list_length; indirect_block_index++)
            {
                if((double_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, double_indirect_block_no)) == NULL)
                {
                    mext2_error("Could not read indirect block %u", double_indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                uint32_t indirect_block_no;
                uint32_t* indirect_block;
                if((indirect_block_no = mext2_le_to_cpu32(double_indirect_block[indirect_block_index])) == EXT2_INVALID_INO)
                {
                    if(mext2_allocate_blocks(sd, double_indirect_block_no / sd->fs.descriptor.ext2.s_blocks_per_group, &indirect_block_no, 1) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Could not allocate indirect block for double indirect block %u", double_indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    if((indirect_block = (uint32_t*)mext2_get_ext2_block(sd, indirect_block_no)) == NULL)
                    {
                        mext2_error("Could not fetch newly allocated indirect block %u for clearing", indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    memset((void*)indirect_block, 0, EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));
                    if(mext2_put_ext2_block(sd, (block512_t*)indirect_block, indirect_block_no) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Could not update newly allocated indirect block %u after clearing", indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    if((double_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, double_indirect_block_no)) == NULL)
                    {
                        mext2_error("Could not read double indirect block %u after allocating indirect block", double_indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    double_indirect_block[indirect_block_index] = mext2_cpu_to_le32(indirect_block_no);
                    if(mext2_put_ext2_block(sd, (block512_t*)double_indirect_block, double_indirect_block_no) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Could not update double indirect block %u after allocating new indirect block %u", double_indirect_block_no, indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    mext2_debug("Allocated new indirect block %u for double indirect block %u", indirect_block_no, double_indirect_block_no);
                }

                if((indirect_block = (uint32_t*)mext2_get_ext2_block(sd, indirect_block_no)) == NULL)
                {
                    mext2_error("Could not fetch indirect block %u", indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                for(uint32_t block_index = (ext2_blocks_count - first_block_index_in_double_indirect) - indirect_block_index * block_addresses_per_block;
                        block_index < block_addresses_per_block
                        && inserted_blocks < block_list_length ; block_index++)
                {
                    indirect_block[block_index] = mext2_cpu_to_le32(block_list[inserted_blocks++]);
                    mext2_debug("Inserted new block %u at index %u", indirect_block[block_index], ext2_blocks_count);
                    ext2_blocks_count++;
                }

                if(mext2_put_ext2_block(sd, (block512_t*)indirect_block, indirect_block_no) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update newly allocated indirect block %u after clearing", indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

            }
        }
        else if(ext2_blocks_count < maximum_inode_blocks)
        {
            uint32_t triple_indirect_block_no;
            uint32_t* triple_indirect_block;
            if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
            {
                mext2_error("Could not read inode at address %u in order to insert new blocks", inode_address.block_address);
                return MEXT2_RETURN_FAILURE;
            }

            if((triple_indirect_block_no = inode->i_triple_indirect_block) == EXT2_INVALID_INO)
            {
                if(mext2_allocate_blocks(sd, inode_address.block_address / sd->fs.descriptor.ext2.s_blocks_per_group, &triple_indirect_block_no, 1) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not allocate triple indirect block for inode at address %u", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                if((triple_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, triple_indirect_block_no)) == NULL)
                {
                    mext2_error("Could not fetch newly allocated triple indirect block %u for clearing", triple_indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                memset((void*)triple_indirect_block, 0, EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));
                if(mext2_put_ext2_block(sd, (block512_t*)triple_indirect_block, triple_indirect_block_no) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update newly allocated triple indirect block %u after clearing", triple_indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
                {
                    mext2_error("Could not read inode at address %u in order to insert new triple indirect", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                inode->i_triple_indirect_block = triple_indirect_block_no;
                if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
                {
                    mext2_error("Could not update inode at address %u after allocating new triple indirect block", inode_address.block_address);
                    return MEXT2_RETURN_FAILURE;
                }

                mext2_debug("Allocated new triple indirect block - %u", triple_indirect_block_no);
            }

            for(uint32_t double_indirect_block_index = (ext2_blocks_count - first_block_index_in_triple_indirect)
                                                            / (block_addresses_per_block * block_addresses_per_block);
                                                            ext2_blocks_count < maximum_inode_blocks
                                                            && inserted_blocks < block_list_length; double_indirect_block_index++)
            {
                if((triple_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, triple_indirect_block_no)) == NULL)
                {
                    mext2_error("Could not read indirect block %u", triple_indirect_block_no);
                    return MEXT2_RETURN_FAILURE;
                }

                uint32_t double_indirect_block_no;
                uint32_t* double_indirect_block;
                if((double_indirect_block_no = mext2_le_to_cpu32(triple_indirect_block[double_indirect_block_index])) == EXT2_INVALID_INO)
                {
                    if(mext2_allocate_blocks(sd, triple_indirect_block_no / sd->fs.descriptor.ext2.s_blocks_per_group, &double_indirect_block_no, 1) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Could not allocate double indirect block for triple indirect block %u", triple_indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    if((double_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, double_indirect_block_no)) == NULL)
                    {
                        mext2_error("Could not fetch newly allocated double indirect block %u for clearing", double_indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    memset((void*)double_indirect_block, 0, EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));
                    if(mext2_put_ext2_block(sd, (block512_t*)double_indirect_block, double_indirect_block_no) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Could not update newly allocated double indirect block %u after clearing", double_indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    if((triple_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, triple_indirect_block_no)) == NULL)
                    {
                        mext2_error("Could not read triple indirect block %u after allocating double indirect block", triple_indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    triple_indirect_block[double_indirect_block_index] = mext2_cpu_to_le32(double_indirect_block_no);
                    if(mext2_put_ext2_block(sd, (block512_t*)triple_indirect_block, triple_indirect_block_no) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Could not update triple indirect block %u after allocating double indirect block %u", triple_indirect_block_no, double_indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    mext2_debug("Allocated new double indirect block %u for triple indirect block %u", double_indirect_block_no, triple_indirect_block_no);
                }

                for(uint32_t indirect_block_index = (ext2_blocks_count - first_block_index_in_triple_indirect
                                                     - double_indirect_block_index * block_addresses_per_block * block_addresses_per_block) / block_addresses_per_block;
                                                     indirect_block_index < block_addresses_per_block
                                                     && inserted_blocks < block_list_length; indirect_block_index++)
                {
                    if((double_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, double_indirect_block_no)) == NULL)
                    {
                        mext2_error("Could not read indirect block %u", double_indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    uint32_t indirect_block_no;
                    uint32_t* indirect_block;
                    if(double_indirect_block[indirect_block_index] == EXT2_INVALID_INO)
                    {
                        if(mext2_allocate_blocks(sd, double_indirect_block_no / sd->fs.descriptor.ext2.s_blocks_per_group, &indirect_block_no, 1) != MEXT2_RETURN_SUCCESS)
                        {
                            mext2_error("Could not allocate indirect block for double indirect block %u", double_indirect_block_no);
                            return MEXT2_RETURN_FAILURE;
                        }

                        if((indirect_block = (uint32_t*)mext2_get_ext2_block(sd, indirect_block_no)) == NULL)
                        {
                            mext2_error("Could not fetch newly allocated indirect block %u for clearing", indirect_block_no);
                            return MEXT2_RETURN_FAILURE;
                        }

                        memset((void*)indirect_block, 0, EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));
                        if(mext2_put_ext2_block(sd, (block512_t*)indirect_block, indirect_block_no) != MEXT2_RETURN_SUCCESS)
                        {
                            mext2_error("Could not update newly allocated indirect block %u after clearing", indirect_block_no);
                            return MEXT2_RETURN_FAILURE;
                        }

                        if((double_indirect_block = (uint32_t*)mext2_get_ext2_block(sd, double_indirect_block_no)) == NULL)
                        {
                            mext2_error("Could not read double indirect block %u after allocating indirect block", double_indirect_block_no);
                            return MEXT2_RETURN_FAILURE;
                        }

                        double_indirect_block[indirect_block_index] = mext2_cpu_to_le32(indirect_block_no);
                        if(mext2_put_ext2_block(sd, (block512_t*)double_indirect_block, double_indirect_block_no) != MEXT2_RETURN_SUCCESS)
                        {
                            mext2_error("Could not update double indirect block %u after allocating new indirect block %u", double_indirect_block_no, indirect_block_no);
                            return MEXT2_RETURN_FAILURE;
                        }

                        mext2_debug("Allocated new indirect block %u for double indirect block %u", indirect_block_no, double_indirect_block_no);
                    }
                    else
                        indirect_block_no = mext2_le_to_cpu32(double_indirect_block[indirect_block_index]);

                    if((indirect_block = (uint32_t*)mext2_get_ext2_block(sd, indirect_block_no)) == NULL)
                    {
                        mext2_error("Could not fetch indirect block %u", indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }

                    for(uint32_t block_index = ext2_blocks_count - first_block_index_in_triple_indirect
                            - double_indirect_block_index * block_addresses_per_block * block_addresses_per_block
                            - indirect_block_index * block_addresses_per_block;
                            block_index < block_addresses_per_block
                            && inserted_blocks < block_list_length ; block_index++)
                    {
                        indirect_block[block_index] = mext2_cpu_to_le32(block_list[inserted_blocks++]);
                        mext2_debug("Inserted new block %u at index %u", indirect_block[block_index], ext2_blocks_count);
                        ext2_blocks_count++;
                    }

                    if(mext2_put_ext2_block(sd, (block512_t*)indirect_block, indirect_block_no) != MEXT2_RETURN_SUCCESS)
                    {
                        mext2_error("Could not update newly allocated indirect block %u after clearing", indirect_block_no);
                        return MEXT2_RETURN_FAILURE;
                    }
                }
            }
        }
    }

    if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
    {
        mext2_error("Could not read inode at address %u in order to insert new blocks", inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    inode->i_blocks = EXT2_BLOCKS_TO_PHYSICAL(ext2_blocks_count, sd->fs.descriptor.ext2.s_log_block_size);

    if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not update inode at address %u", inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    if(inserted_blocks < block_list_length)
    {
        mext2_error("Could not insert %u blocks to inode at block %u", block_list_length, inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }
    else
        return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t mext2_inode_insert_new_blocks(struct mext2_sd* sd, uint32_t inode_no, uint32_t* block_list, uint16_t block_list_length)
{
    struct mext2_ext2_inode_address inode_address;
    if((inode_address = mext2_get_ext2_inode_address(sd, inode_no)).block_address == EXT2_INVALID_BLOCK_NO)
    {
        mext2_error("Could not read inode %u, address", inode_no);
        return MEXT2_RETURN_FAILURE;
    }

    return mext2_inode_address_insert_new_blocks(sd, inode_address, block_list, block_list_length);
}

uint8_t mext2_ext2_inode_extend(struct mext2_sd* sd, struct mext2_ext2_inode_address inode_address, uint64_t new_size)
{
    mext2_debug("Attempting to resize inode to size %u", new_size);
    struct mext2_ext2_inode* inode;
    if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
    {
        mext2_error("Could not read inode at block block %u", inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    if(new_size < inode->i_size)
    {
        mext2_warning("Tried to extend inode of size %llu to %llu", inode->i_size, new_size);
        return MEXT2_RETURN_SUCCESS;
    }

    uint32_t standard_block_no_list[MEXT2_MAX_BLOCK_NO_LIST_SIZE];
    uint32_t* block_list = &standard_block_no_list[0];
    uint16_t block_list_max_size = MEXT2_MAX_BLOCK_NO_LIST_SIZE;

    /* check if we have any additional space to store block numbers for allocation */
    if(MEXT2_USEFULL_BLOCKS_SIZE * sizeof(block512_t) - EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) != 0)
    {
        block_list = (uint32_t*)(mext2_usefull_blocks + EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(block512_t));
        block_list_max_size = (MEXT2_USEFULL_BLOCKS_SIZE * sizeof(block512_t) - EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size)) / sizeof(block_list[0]);
    }

    uint16_t new_block_size = (new_size + EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) - 1)
            / EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size);
    uint16_t blocks_to_allocate =  new_block_size
            - EXT2_INODE_BLOCK_SIZE(inode->i_blocks, sd->fs.descriptor.ext2.s_log_block_size);
    uint16_t blocks_allocated = 0;

    while(blocks_allocated < blocks_to_allocate)
    {
        uint16_t current_blocks_to_allocate = blocks_to_allocate - blocks_allocated < block_list_max_size ? blocks_to_allocate - blocks_allocated : block_list_max_size;

        uint16_t current_blocks_allocated;
        if((current_blocks_allocated = mext2_allocate_blocks(sd, inode_address.block_address / sd->fs.descriptor.ext2.s_blocks_per_group,
                 block_list, current_blocks_to_allocate)) < current_blocks_to_allocate)
        {
            mext2_error("Could only allocate %u blocks when needed %u", current_blocks_allocated, current_blocks_to_allocate);
            if(mext2_deallocate_blocks(sd, block_list, current_blocks_allocated) != MEXT2_RETURN_SUCCESS)
                mext2_error("Could not free blocks after allocating insufficient amount of blocks");

            return MEXT2_RETURN_FAILURE;
        }
        mext2_inode_address_insert_new_blocks(sd, inode_address, block_list, current_blocks_allocated);
        blocks_allocated += current_blocks_allocated;
    }

    if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
    {
        mext2_error("Could not read inode at block %u", inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    inode->i_size = new_size;

    mext2_debug("Resized inode to size %llu, i_blocks: %u, first inode direct block %u", new_size, inode->i_blocks, inode->i_direct_block[0]);
    mext2_log("Inode direct blocks: %u %u %u %u %u %u %u %u %u %u %u",
            inode->i_direct_block[0],
            inode->i_direct_block[1],
            inode->i_direct_block[2],
            inode->i_direct_block[3],
            inode->i_direct_block[4],
            inode->i_direct_block[5],
            inode->i_direct_block[6],
            inode->i_direct_block[8],
            inode->i_direct_block[9],
            inode->i_direct_block[10]
            );
    if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Could not write to inode at block %u for size update", inode_address.block_address);
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}

uint8_t mext2_ext2_insert_dir_entry(struct mext2_sd* sd, uint32_t dir_inode, struct mext2_ext2_dir_entry_head* dir_entry, char* name)
{
    struct mext2_ext2_inode_address inode_address;
    struct mext2_ext2_inode* inode;
    if((inode_address = mext2_get_ext2_inode_address(sd, dir_inode)).block_address == EXT2_INVALID_BLOCK_NO)
    {
        mext2_error("Could not read inode address for dir inode %u", dir_inode);
        return MEXT2_RETURN_FAILURE;
    }

    if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
    {
        mext2_error("Could not read dir inode %u", dir_inode);
        return MEXT2_RETURN_FAILURE;
    }

    if((inode->i_mode & EXT2_FORMAT_MASK) != EXT2_S_IFDIR)
    {
        mext2_error("Inode %u is not an dir inode", dir_inode);
        return MEXT2_RETURN_FAILURE;
    }

    uint32_t inode_index = EXT2_INODE_BLOCK_SIZE(inode->i_blocks, sd->fs.descriptor.ext2.s_log_block_size) - 1;
    block512_t* block;
    uint32_t block_no;
    if((block_no = mext2_get_data_block_by_inode_index(sd, inode, inode_index)) == EXT2_INVALID_BLOCK_NO)
    {
        mext2_error("Could not find last directory block(%u) in inode %u", inode_index, dir_inode);
        return MEXT2_RETURN_FAILURE;
    }

    if((block = mext2_get_ext2_block(sd, block_no)) == NULL)
    {
        mext2_error("Could not read inode block at index: %u, block number: %u", inode_index, block_no);
        return MEXT2_RETURN_FAILURE;
    }

    struct mext2_ext2_dir_entry* current_dir_entry = (struct mext2_ext2_dir_entry*) block;
    uint16_t dir_entry_pos = 0;

    while(dir_entry_pos + mext2_le_to_cpu16(current_dir_entry->head.rec_len) < EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) - 1)
    {
        dir_entry_pos += mext2_le_to_cpu16(current_dir_entry->head.rec_len);
        current_dir_entry = (struct mext2_ext2_dir_entry*)((uint8_t*)block + dir_entry_pos);
    }

    uint16_t current_dir_entry_minimal_size = sizeof(struct mext2_ext2_dir_entry_head) + mext2_le_to_cpu16(current_dir_entry->head.name_len);
    current_dir_entry_minimal_size += (current_dir_entry_minimal_size % 4);
    if(EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) - (dir_entry_pos + current_dir_entry_minimal_size) >= sizeof(struct mext2_ext2_dir_entry_head) + dir_entry->name_len)
    {
        // dir_entry can fit into current block
        current_dir_entry->head.rec_len = mext2_cpu_to_le16(current_dir_entry_minimal_size);

        current_dir_entry = (struct mext2_ext2_dir_entry*)((uint8_t*)current_dir_entry + current_dir_entry_minimal_size);
        dir_entry_pos += current_dir_entry_minimal_size;
        current_dir_entry->head.inode = mext2_cpu_to_le32(dir_entry->inode);
        current_dir_entry->head.name_len = dir_entry->name_len;
        current_dir_entry->head.file_type = dir_entry->file_type;
        current_dir_entry->head.rec_len = mext2_cpu_to_le16(EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) - dir_entry_pos);
        strncpy(&current_dir_entry->name[0], name, (size_t)dir_entry->name_len);
    }
    else
    {
        //dir entry can't fit
        if(mext2_allocate_blocks(sd, block_no / sd->fs.descriptor.ext2.s_blocks_per_group, &block_no, 1) == 0)
        {
            mext2_error("Could not allocate new block for dir inode %u", dir_inode);
            return MEXT2_RETURN_FAILURE;
        }

        if(mext2_inode_address_insert_new_blocks(sd, inode_address, &block_no, 1) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Could not insert newly allocated block %u to inode %u", block_no, dir_inode);
            return MEXT2_RETURN_FAILURE;
        }

        if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
        {
            mext2_error("Could not fetch inode %u", dir_inode);
            return MEXT2_RETURN_FAILURE;
        }

        inode->i_blocks += EXT2_BLOCKS_TO_PHYSICAL(1, sd->fs.descriptor.ext2.s_log_block_size);

        if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Could not update inode %u", dir_inode);
            return MEXT2_RETURN_FAILURE;
        }

        if((block = mext2_get_ext2_block(sd, block_no)) == NULL)
        {
            mext2_error("Could not fetch newly allocated block %u for dir inode %u", block_no, dir_inode);
            return MEXT2_RETURN_FAILURE;
        }

        memset((void*)block, 0, EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));
        current_dir_entry = (struct mext2_ext2_dir_entry*) block;
        current_dir_entry->head.inode = mext2_cpu_to_le32(dir_entry->inode);
        current_dir_entry->head.name_len = dir_entry->name_len;
        current_dir_entry->head.file_type = dir_entry->file_type;
        current_dir_entry->head.rec_len = mext2_cpu_to_le16(EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));
        strncpy(&current_dir_entry->name[0], name, (size_t)dir_entry->name_len);
    }

    if(mext2_put_ext2_block(sd, block, block_no) == MEXT2_RETURN_FAILURE)
    {
        mext2_error("Could not update dir inode: %u block no: %u", dir_inode, block_no);
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}

uint32_t mext2_get_regular_file_inode(struct mext2_sd* sd, char* path, mext2_bool create_not_found)
{
    if(path[0] != MEXT2_PATH_SEPARATOR)
    {
        mext2_error("Path doesn't start with '/'");
        mext2_errno = MEXT2_INVALID_PATH;
        return EXT2_INVALID_INO;
    }

    char* token = path_tokenize(path);
    size_t token_length;
    char delimiter[] = { MEXT2_PATH_SEPARATOR, '\0' };
    char name[MEXT2_MAX_FILENAME_LENGTH + 1];

    uint32_t previous_inode_no;
    uint32_t current_inode_no = EXT2_ROOT_INO;

    mext2_bool lookup_failed = MEXT2_FALSE;
    do
    {
        previous_inode_no = current_inode_no;
        token_length = strcspn(token, delimiter);
        strncpy(name, token, token_length);
        name[token_length] = '\0';
        if((current_inode_no = mext2_inode_no_lookup_from_dir_inode(sd, previous_inode_no, name)) == EXT2_INVALID_INO)
        {
            mext2_debug("Lookup failed in inode: %u", previous_inode_no);
            if(mext2_errno != MEXT2_INVALID_PATH)
            {
                mext2_error("Error have occured during recognition of name %s", name);
                return EXT2_INVALID_INO;
            }
            lookup_failed = MEXT2_TRUE;
        }

    }
    while(!lookup_failed && (token = path_tokenize(NULL)));

    if(lookup_failed && create_not_found)
    {
        if(path_tokenize(NULL) != NULL)
        {
            mext2_errno = MEXT2_INVALID_PATH;
            return EXT2_INVALID_INO;
        }

        if((current_inode_no = allocate_ext2_inode(sd, previous_inode_no / sd->fs.descriptor.ext2.s_inodes_per_group)) == EXT2_INVALID_INO)
        {
            mext2_error("Could not allocate inode for file creation");
            return EXT2_INVALID_INO;
        }

        struct mext2_ext2_dir_entry_head dir_entry;
        dir_entry.inode = current_inode_no;

        struct mext2_ext2_inode* inode;
        struct mext2_ext2_inode_address inode_address;
        if((inode_address = mext2_get_ext2_inode_address(sd, dir_entry.inode)).block_address == EXT2_INVALID_BLOCK_NO)
        {
            mext2_error("Could not fetch newly allocated inode %u", dir_entry.inode);
            return MEXT2_RETURN_FAILURE;
        }

        if((inode = mext2_get_ext2_inode(sd, inode_address)) == NULL)
        {
            mext2_error("Could not read newly allocated inode %u", dir_entry.inode);
            return MEXT2_RETURN_FAILURE;
        }

        memset((void*)inode, 0, sizeof(struct mext2_ext2_inode));
        inode->i_mode = EXT2_S_IFREG | EXT2_S_IRUSR | EXT2_S_IWUSR;
        inode->i_links_count = 1;

        if(mext2_put_ext2_inode(sd, inode, inode_address) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Could not write to newly allocated inode %u", dir_entry.inode);
            return MEXT2_RETURN_FAILURE;
        }

        if(sd->fs.descriptor.ext2.s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE)
            dir_entry.file_type = EXT2_FT_REG_FILE;
        else
            dir_entry.file_type = 0;

        dir_entry.name_len = token_length;
        if(mext2_ext2_insert_dir_entry(sd, previous_inode_no, &dir_entry, &name[0]) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Could not insert dir entry to inode %u", previous_inode_no);
            return MEXT2_RETURN_FAILURE;
        }
    }

    return current_inode_no;
}

uint8_t mext2_ext2_sd_parser(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock)
{
    uint32_t rev_level = mext2_le_to_cpu32(superblock -> s_rev_level);

    sd->fs.fs_flags = MEXT2_CLEAN;
    if(mext2_le_to_cpu16(superblock->s_state) != EXT2_VALID_FS)
    {
        mext2_warning("Filesystem contains errors and shouldn't be used, fs_state: %#x", mext2_le_to_cpu16(superblock->s_state));
        sd->fs.fs_flags |= MEXT2_FS_ERRORS;
//      mext2_errno = MEXT2_ERRONEOUS_FS;
//      return MEXT2_RETURN_FAILURE;  // to rethink  if we should return here or not
    }

    sd->fs.write_open_file_counter = 0;

    sd->fs.descriptor.ext2.s_inodes_count = superblock->s_inodes_count;
    sd->fs.descriptor.ext2.s_blocks_count = superblock->s_blocks_count;
    sd->fs.descriptor.ext2.s_free_blocks_count = superblock->s_free_blocks_count;
    sd->fs.descriptor.ext2.s_free_inodes_count = superblock->s_free_inodes_count;
    sd->fs.descriptor.ext2.s_first_data_block = superblock->s_first_data_block;
    sd->fs.descriptor.ext2.s_log_block_size = superblock->s_log_block_size;
    sd->fs.descriptor.ext2.s_blocks_per_group = superblock->s_blocks_per_group;
    sd->fs.descriptor.ext2.s_inodes_per_group = superblock->s_inodes_per_group;

    if(rev_level == EXT2_GOOD_OLD_REV)
    {
        sd->fs.descriptor.ext2.s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
        sd->fs.descriptor.ext2.s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
        sd->fs.descriptor.ext2.s_feature_compat = EXT2_NO_FEATURES;
        sd->fs.descriptor.ext2.s_feature_incompat = EXT2_NO_FEATURES;
        sd->fs.descriptor.ext2.s_feature_ro_compat = EXT2_NO_FEATURES;
    }
    else
    {
        sd->fs.descriptor.ext2.s_first_ino = superblock->s_first_ino;
        sd->fs.descriptor.ext2.s_inode_size = superblock->s_inode_size;
        sd->fs.descriptor.ext2.s_feature_compat = superblock->s_feature_compat;
        sd->fs.descriptor.ext2.s_feature_incompat = superblock->s_feature_incompat;
        sd->fs.descriptor.ext2.s_feature_ro_compat = superblock->s_feature_ro_compat;
    }

    if(mext2_is_big_endian())
    {
        sd->fs.descriptor.ext2.s_inodes_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_inodes_count);
        sd->fs.descriptor.ext2.s_blocks_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_blocks_count);
        sd->fs.descriptor.ext2.s_free_blocks_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_free_blocks_count);
        sd->fs.descriptor.ext2.s_free_inodes_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_free_inodes_count);
        sd->fs.descriptor.ext2.s_first_data_block = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_first_data_block);
        sd->fs.descriptor.ext2.s_log_block_size = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_log_block_size);
        sd->fs.descriptor.ext2.s_blocks_per_group = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_blocks_per_group);
        sd->fs.descriptor.ext2.s_inodes_per_group = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_inodes_per_group);

        if(rev_level == EXT2_DYNAMIC_REV)
        {
            sd->fs.descriptor.ext2.s_first_ino = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_first_ino);
            sd->fs.descriptor.ext2.s_inode_size = mext2_flip_endianess16(sd->fs.descriptor.ext2.s_inode_size);
            sd->fs.descriptor.ext2.s_feature_compat = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_feature_compat);
            sd->fs.descriptor.ext2.s_feature_incompat = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_feature_incompat);
            sd->fs.descriptor.ext2.s_feature_ro_compat = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_feature_ro_compat);
        }
    }

    if(EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) > sizeof(block512_t) * MEXT2_USEFULL_BLOCKS_SIZE)
    {
        mext2_error("Unsupported block size: %d, maximal supported block size: %d", EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size), sizeof(block512_t) * MEXT2_USEFULL_BLOCKS_SIZE );
        mext2_errno = MEXT2_TOO_LARGE_BLOCK;
        return MEXT2_RETURN_FAILURE;
    }
    else
        mext2_log("Ext2 block size: %d", EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));

    if(sd->fs.descriptor.ext2.s_feature_compat & ~(MEXT2_FEATURES_COMPAT))
    {
        mext2_log("Detected unsupported compatible features: %#x", sd->fs.descriptor.ext2.s_feature_compat);
    }

    if(sd->fs.descriptor.ext2.s_feature_incompat & ~(MEXT2_FEATURES_INCOMPAT))
    {
        mext2_error("Detected incompatible features: %#x", sd->fs.descriptor.ext2.s_feature_incompat);
        return MEXT2_RETURN_FAILURE;
    }

    if(sd->fs.descriptor.ext2.s_feature_ro_compat & ~(MEXT2_FEATURES_RO_COMPAT))
    {
        mext2_warning("Detected unsupported read-only compatible features: %#x", sd->fs.descriptor.ext2.s_feature_ro_compat);
        sd->fs.fs_flags |=  MEXT2_READ_ONLY;
    }
    else
    {
        mext2_log("Filesystem can be mounted read-write");
    }

    sd->fs.open_strategy = mext2_ext2_open;
    sd->fs.close_strategy = mext2_ext2_close;
    sd->fs.read_strategy = mext2_ext2_read;
    sd->fs.write_strategy = mext2_ext2_write;
    sd->fs.seek_strategy = mext2_ext2_seek;
    sd->fs.eof_strategy = mext2_ext2_eof;

    return MEXT2_RETURN_SUCCESS;
}
