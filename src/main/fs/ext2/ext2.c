#include <string.h>
#include "ext2.h"
#include "superblock.h"
#include "inode.h"
#include "block_group_descriptor.h"
#include "sd.h"
#include "common.h"
#include "debug.h"
#include "dir_entry.h"
#include "ext2/file.h"


uint8_t ext2_errno = EXT2_NO_ERRORS;

block512_t* mext2_get_ext2_block(struct mext2_sd* sd, uint32_t block_no)
{
    const uint16_t ext2_block_block_size = EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size) / sizeof(block512_t);
    uint32_t block_block_address = sd->partition_block_addr + block_no * ext2_block_block_size;


    if(mext2_read_blocks(sd, block_block_address, mext2_usefull_blocks, ext2_block_block_size) != MEXT2_RETURN_SUCCESS)
    {
        mext2_error("Can't read block no %d, at address %#x", block_no, block_block_address);
        return NULL;
    }

    return &mext2_usefull_blocks[0];
}

STATIC void correct_block_group_descriptor_endianess(struct mext2_ext2_block_group_descriptor* bgd)
{
    bgd->bg_block_bitmap = mext2_flip_endianess32(bgd->bg_block_bitmap);
    bgd->bg_inode_bitmap = mext2_flip_endianess32(bgd->bg_inode_bitmap);
    bgd->bg_inode_table = mext2_flip_endianess32(bgd->bg_inode_table);
    bgd->bg_free_blocks_count = mext2_flip_endianess16(bgd->bg_free_blocks_count);
    bgd->bg_free_inodes_count = mext2_flip_endianess16(bgd->bg_free_inodes_count);
    bgd->bg_used_dirs_count = mext2_flip_endianess16(bgd->bg_used_dirs_count);
    bgd->bg_pad = mext2_flip_endianess16(bgd->bg_pad);
}

#define INVALID_INODE_ADDRESS 0
#define BLOCK_GROUP_DESCRIPTOR_TABLE_SUPERBLOCK_OFFSET 1

struct mext2_inode_address mext2_inode_no_to_addr(struct mext2_sd* sd, uint32_t inode_no)
{
    struct mext2_inode_address address;
    uint32_t block_group_descriptor_block_no = ((inode_no - 1) / sd->fs.descriptor.ext2.s_inodes_per_group)
            * sizeof(struct mext2_ext2_block_group_descriptor) / EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size)
            + BLOCK_GROUP_DESCRIPTOR_TABLE_SUPERBLOCK_OFFSET + sd->fs.descriptor.ext2.s_first_data_block;

    uint32_t block_group_in_block_offset = (((inode_no - 1) / sd->fs.descriptor.ext2.s_inodes_per_group) // block_group number
            * sizeof(struct mext2_ext2_block_group_descriptor)) % EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size); // find offset in block

    block512_t* block;
    if((block = mext2_get_ext2_block(sd, block_group_descriptor_block_no)) == NULL)
    {
        mext2_error("Could not read block group descriptor", block_group_descriptor_block_no);
        address.block_address = INVALID_INODE_ADDRESS;
        ext2_errno = EXT2_READ_ERROR;
        return address;
    }
    struct mext2_ext2_block_group_descriptor* bgd = (struct mext2_ext2_block_group_descriptor*) (((uint8_t*)block) + block_group_in_block_offset);

    if(mext2_is_big_endian())
        correct_block_group_descriptor_endianess(bgd);

    address.block_inode_bitmap_address = bgd->bg_block_bitmap;
    address.inode_local_index = (inode_no - 1) % sd->fs.descriptor.ext2.s_inodes_per_group;

    address.block_address = (bgd->bg_inode_table + ((address.inode_local_index
            * sd->fs.descriptor.ext2.s_inode_size)
            / EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size)));
    address.block_offset = ((address.inode_local_index * sd->fs.descriptor.ext2.s_inode_size) % EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size));

    return address;
}

STATIC void correct_inode_endianess(struct mext2_ext2_inode* inode)
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

STATIC void correct_dir_entry_head_endianess(struct mext2_ext2_dir_entry_head* head)
{
    head->inode = mext2_flip_endianess32(head->inode);
    head->rec_len = mext2_flip_endianess16(head->rec_len);
}

#define NO_USE_CACHE 0
#define SPLIT_HEAD 1
#define SPLIT_NAME 2
#define LONG_REC   3

STATIC uint32_t find_inode_no_direct_block(mext2_sd* sd, uint32_t direct_block_address, char* name)
{

    mext2_debug("Looking for name '%s' in direct block at address %#x", name, direct_block_address);
    block512_t* block;
    if((block = mext2_get_ext2_block(sd, direct_block_address)) == NULL)
    {
        ext2_errno = EXT2_READ_ERROR;
        mext2_error("Could not read direct block");
        return EXT2_INVALID_INO;
    }
    struct mext2_ext2_dir_entry* dir_entry = (struct mext2_ext2_dir_entry*)block;
    uint16_t pos = 0;

    while(pos < EXT2_BLOCK_SIZE(sd->fs.descriptor.ext2.s_log_block_size))
    {
        if(mext2_is_big_endian())
            correct_dir_entry_head_endianess(&dir_entry->head);
        mext2_debug("Inode: %d, Record length: %d, Name length: %d, File type: %d, Name: %*s",
            dir_entry->head.inode,
            dir_entry->head.rec_len,
            dir_entry->head.name_len,
            dir_entry->head.file_type,
            dir_entry->head.name_len,
            dir_entry->name
        );
        if(strncmp(name, &dir_entry->name[0], dir_entry->head.name_len) == 0)
        {
            mext2_log("Inode number %d found for: '%s'", dir_entry->head.inode, name);
            return dir_entry->head.inode;
        }
        pos += dir_entry->head.rec_len;
        dir_entry = (struct mext2_ext2_dir_entry*)(((uint8_t*)block) + pos);
    }

    ext2_errno = EXT2_INVALID_PATH;
    return EXT2_INVALID_INO;
}

STATIC uint32_t find_inode_no_indirect_block(mext2_sd* sd, uint32_t indirect_block_address, char* name)
{
    mext2_debug("Looking for name '%s' in indirect block at address %#x", name, indirect_block_address);
    uint32_t* direct_blocks;
    uint32_t inode_no;

    uint32_t direct_block_index = 0;
    for(direct_block_index = 0; direct_block_index; direct_block_index++)
    {
        if((direct_blocks = (uint32_t*)mext2_get_ext2_block(sd, indirect_block_address)) == NULL)
        {
            ext2_errno = EXT2_READ_ERROR;
            mext2_error("Could not read indirect block");
            return EXT2_INVALID_INO;
        }
        if(direct_blocks[direct_block_index] == 0)
        {
            ext2_errno = EXT2_INVALID_PATH;
            return EXT2_INVALID_INO;
        }

        ext2_errno = EXT2_NO_ERRORS;
        if((inode_no = find_inode_no_direct_block(sd, mext2_le_to_cpu32(direct_blocks[direct_block_index]), name)) != EXT2_INVALID_INO)
            return inode_no;

        if(ext2_errno != EXT2_INVALID_PATH)
            return EXT2_INVALID_INO;

    }

    ext2_errno = EXT2_INVALID_PATH;
    return EXT2_INVALID_INO;
}

STATIC uint32_t find_inode_no_double_indirect_block(mext2_sd* sd, uint32_t double_indirect_block_address, char* name)
{
    mext2_debug("Looking for name '%s' in double indirect block at address %#x", name, double_indirect_block_address);
    uint32_t* indirect_blocks;
    uint32_t inode_no;

    uint32_t indirect_block_index = 0;
    for(indirect_block_index = 0; indirect_block_index; indirect_block_index++)
    {
        if((indirect_blocks = (uint32_t*)mext2_get_ext2_block(sd, double_indirect_block_address)) == NULL)
        {
            ext2_errno = EXT2_READ_ERROR;
            mext2_error("Could not read double block");
            return EXT2_INVALID_INO;
        }
        if(indirect_blocks[indirect_block_index] == 0)
        {
            mext2_error("Could not find inode number for specified path");
            ext2_errno = EXT2_INVALID_PATH;
            return EXT2_INVALID_INO;
        }

        ext2_errno = EXT2_NO_ERRORS;
        if((inode_no = find_inode_no_indirect_block(sd, mext2_le_to_cpu32(indirect_blocks[indirect_block_index]), name)) != EXT2_INVALID_INO)
            return inode_no;

        if(ext2_errno != EXT2_INVALID_PATH)
            return EXT2_INVALID_INO;
    }

    ext2_errno = EXT2_INVALID_PATH;
    return EXT2_INVALID_INO;
}

STATIC uint32_t find_inode_no_triple_indirect_block(mext2_sd* sd, uint32_t triple_indirect_block_address, char* name)
{
    mext2_debug("Looking for name '%s' in triple indirect block at address %#x", name, triple_indirect_block_address);
    uint32_t* double_indirect_blocks;
    uint32_t inode_no;

    uint32_t double_indirect_block_index = 0;
    for(double_indirect_block_index = 0; double_indirect_block_index; double_indirect_block_index++)
    {
        if((double_indirect_blocks = (uint32_t*)mext2_get_ext2_block(sd, triple_indirect_block_address)) == NULL)
        {
            ext2_errno = EXT2_READ_ERROR;
            mext2_error("Could not read triple indirect block");
            return EXT2_INVALID_INO;
        }
        if(double_indirect_blocks[double_indirect_block_index] == 0)
        {
            mext2_error("Could not find inode number for specified path");
            ext2_errno = EXT2_INVALID_PATH;
            return EXT2_INVALID_INO;
        }

        ext2_errno = EXT2_NO_ERRORS;
        if((inode_no = find_inode_no_double_indirect_block(sd, mext2_le_to_cpu32(double_indirect_blocks[double_indirect_block_index]), name)) != EXT2_INVALID_INO)
            return inode_no;

        if(ext2_errno != EXT2_INVALID_PATH)
            return EXT2_INVALID_INO;
    }

    mext2_error("Could not find inode number for specified path");
    ext2_errno = EXT2_INVALID_PATH;
    return EXT2_INVALID_INO;
}

#define I_INDIRECT_BLOCK_INDEX 13
#define I_DOUBLE_INDIRECT_BLOCK_INDEX 14
#define I_TRIPLE_INDIRECT_BLOCK_INDEX 15

uint32_t mext2_inode_no_lookup_from_dir_inode(struct mext2_sd* sd, uint32_t dir_inode_no, char* name)
{
    struct mext2_inode_address inode_address = mext2_inode_no_to_addr(sd, dir_inode_no);

    if(inode_address.block_address == INVALID_INODE_ADDRESS)
    {
        return EXT2_INVALID_INO;
    }

    mext2_debug("Inode block address: %#hx", inode_address.block_address);

    block512_t* block;
    if((block = mext2_get_ext2_block(sd, inode_address.block_address)) == NULL)
    {
        mext2_error("Could not read inode %d", dir_inode_no);
        ext2_errno = EXT2_READ_ERROR;
        return EXT2_INVALID_INO;
    }

    struct mext2_ext2_inode* inode = (struct mext2_ext2_inode*)(((uint8_t*)block) + inode_address.block_offset);

    if(mext2_is_big_endian())
        correct_inode_endianess(inode);

    if((inode->i_mode & EXT2_FORMAT_MASK) != EXT2_S_IFDIR)
    {
        mext2_error("Inode no: %d is not a directory inode", dir_inode_no);
        ext2_errno = EXT2_INVALID_PATH;
        return EXT2_INVALID_INO;
    }

    mext2_debug("Inode mode: %#hx", inode->i_mode);
    mext2_debug("Inode i_size: %#x", inode->i_size);
    mext2_debug("Inode i_blocks: %d", inode->i_blocks);


    uint32_t blocks[15];
    uint32_t used_blocks = inode->i_blocks / (2 << sd->fs.descriptor.ext2.s_log_block_size);

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
            if(ext2_errno != EXT2_INVALID_PATH)
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
            if(ext2_errno != EXT2_INVALID_PATH)
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
                if(ext2_errno != EXT2_INVALID_PATH)
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
                    if(ext2_errno != EXT2_INVALID_PATH)
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

        if(token_beggining != '\0')
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
    if(token_beggining != '\0')
        return token_beggining;
    else
        return NULL;

}
uint32_t mext2_inode_no_lookup(struct mext2_sd* sd, char* path)
{
    if(path[0] != MEXT2_PATH_SEPARATOR)
    {
        mext2_error("Path doesn't start with '/'");
        ext2_errno = EXT2_INVALID_PATH;
        return EXT2_INVALID_INO;
    }

    char* token = path_tokenize(path);
    char delimiter[] = { MEXT2_PATH_SEPARATOR, '\0' };
    char name[MAX_FILENAME_LENGTH + 1];

    uint32_t current_inode_no = EXT2_ROOT_INO;

    do
    {
        size_t token_length = strcspn(token, delimiter);
        strncpy(name, token, token_length);
        name[token_length] = '\0';
        if((current_inode_no = mext2_inode_no_lookup_from_dir_inode(sd, current_inode_no, name)) == EXT2_INVALID_INO)
        {
            return EXT2_INVALID_INO;
        }
    }
    while((token = path_tokenize(NULL)));

    return current_inode_no;
}

uint8_t mext2_ext2_sd_parser(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock)
{
    uint32_t rev_level = mext2_le_to_cpu32(superblock -> s_rev_level);

    sd->fs.open_file_counter = 0;

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
        sd->fs.ro_flag = MEXT2_TRUE;
    }
    else
    {
        mext2_log("Filesystem can be mounted read-write");
        sd->fs.ro_flag = MEXT2_FALSE;
    }

    sd->fs.open_strategy = mext2_ext2_open;
    sd->fs.close_strategy = mext2_ext2_close;
    sd->fs.read_strategy = mext2_ext2_read;
    sd->fs.write_strategy = mext2_ext2_write;
    sd->fs.seek_strategy = mext2_ext2_seek;
    sd->fs.eof_strategy = mext2_ext2_eof;

    return MEXT2_RETURN_SUCCESS;
}
