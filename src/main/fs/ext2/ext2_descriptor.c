#include "ext2_descriptor.h"
#include "superblock.h"
#include "sd.h"
#include "common.h"
#include "ext2/file.h"

void mext2_ext2_sd_filler(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock)
{
    sd->fs.open_file_counter = 0;
    sd->fs.open_strategy = mext2_ext2_open;

    sd->fs.descriptor.ext2.s_inodes_count = superblock->s_inodes_count;
    sd->fs.descriptor.ext2.s_blocks_count = superblock->s_blocks_count;
    sd->fs.descriptor.ext2.s_free_blocks_count = superblock->s_free_blocks_count;
    sd->fs.descriptor.ext2.s_free_inodes_count = superblock->s_free_inodes_count;
    sd->fs.descriptor.ext2.s_log_block_size = superblock->s_log_block_size;
    sd->fs.descriptor.ext2.s_blocks_per_group = superblock->s_blocks_per_group;
    sd->fs.descriptor.ext2.s_inodes_per_group = superblock->s_inodes_per_group;
    sd->fs.descriptor.ext2.s_first_ino = superblock->s_first_ino;
    sd->fs.descriptor.ext2.s_inode_size = superblock->s_inode_size;

    if(mext2_is_big_endian())
    {
        sd->fs.descriptor.ext2.s_inodes_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_inodes_count);
        sd->fs.descriptor.ext2.s_blocks_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_blocks_count);
        sd->fs.descriptor.ext2.s_free_blocks_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_free_blocks_count);
        sd->fs.descriptor.ext2.s_free_inodes_count = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_free_inodes_count);
        sd->fs.descriptor.ext2.s_log_block_size = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_log_block_size);
        sd->fs.descriptor.ext2.s_blocks_per_group = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_blocks_per_group);
        sd->fs.descriptor.ext2.s_inodes_per_group = mext2_flip_endianess32(sd->fs.descriptor.ext2.s_inodes_per_group);
        sd->fs.descriptor.ext2.s_first_ino = mext2_flip_endianess16(sd->fs.descriptor.ext2.s_first_ino);
    }
}
