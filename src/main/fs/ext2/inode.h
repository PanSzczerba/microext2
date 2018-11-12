#ifndef MEXT2_FS_EXT2_INODE_H
#define MEXT2_FS_EXT2_INODE_H

#include <stdint.h>
#include "common.h"

/**** BLOCK INODE ARRAY VALUES ****/
#define I_INDIRECT_BLOCK_INDEX 12
#define I_DOUBLE_INDIRECT_BLOCK_INDEX 13
#define I_TRIPLE_INDIRECT_BLOCK_INDEX 14

/**** INODE VALUES ****/
#define EXT2_INVALID_INO   0
#define EXT2_BAD_INO         1
#define EXT2_ROOT_INO        2
#define EXT2_ACL_IDX_INO     3
#define EXT2_ACL_DATA_INO    4
#define EXT2_BOOT_LOADER_INO 5
#define EXT2_UNDEL_DIR_INO   6

/**** file format ****/
#define EXT2_S_IFSOCK   0xC000
#define EXT2_S_IFLNK    0xA000
#define EXT2_S_IFREG    0x8000
#define EXT2_S_IFBLK    0x6000
#define EXT2_S_IFDIR    0x4000
#define EXT2_S_IFCHR    0x2000
#define EXT2_S_IFIFO    0x1000

#define EXT2_FORMAT_MASK 0xf000
/**** process execution user/group override ****/
#define EXT2_S_ISUID    0x0800
#define EXT2_S_ISGID    0x0400
#define EXT2_S_ISVTX    0x0200

/**** access rights ****/
#define EXT2_S_IRUSR    0x0100
#define EXT2_S_IWUSR    0x0080
#define EXT2_S_IXUSR    0x0040
#define EXT2_S_IRGRP    0x0020
#define EXT2_S_IWGRP    0x0010
#define EXT2_S_IXGRP    0x0008
#define EXT2_S_IROTH    0x0004
#define EXT2_S_IWOTH    0x0002
#define EXT2_S_IXOTH    0x0001

#define EXT2_RIGHTS_MASK 0x0fff

struct mext2_ext2_inode
{
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_direct_block[12];
    uint32_t i_indirect_block;
    uint32_t i_double_indirect_block;
    uint32_t i_triple_indirect_block;
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];

} __mext2_packed;

struct mext2_ext2_inode_address
{
    uint32_t block_address;
    uint16_t block_offset;
};

#endif
