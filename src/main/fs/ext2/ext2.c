#include "ext2.h"
#include "superblock.h"
#include "sd.h"
#include "common.h"
#include "debug.h"
#include "ext2/file.h"

uint8_t mext2_ext2_sd_parser(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock)
{
    uint32_t rev_level = mext2_le_to_cpu32(superblock -> s_rev_level);

    sd->fs.open_file_counter = 0;

    sd->fs.descriptor.ext2.s_inodes_count = superblock->s_inodes_count;
    sd->fs.descriptor.ext2.s_blocks_count = superblock->s_blocks_count;
    sd->fs.descriptor.ext2.s_free_blocks_count = superblock->s_free_blocks_count;
    sd->fs.descriptor.ext2.s_free_inodes_count = superblock->s_free_inodes_count;
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

    if(sd->fs.descriptor.ext2.s_feature_incompat & ~(MEXT2_FEATURES_INCOMPAT))
    {
        mext2_error("Detected incompatible features: 0x%x", sd->fs.descriptor.ext2.s_feature_incompat);
        return MEXT2_RETURN_FAILURE;
    }

    if(sd->fs.descriptor.ext2.s_feature_ro_compat & ~(MEXT2_FEATURES_RO_COMPAT))
    {
        mext2_warning("Detected unsupported read-only compatible features: 0x%x", sd->fs.descriptor.ext2.s_feature_ro_compat);
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
