#ifndef MEXT2_FS_EXT2_H
#define MEXT2_FS_EXT2_H

#include <stdint.h>

#define EXT2_SUPERBLOCK_PARTITION_BLOCK_OFFSET 2 // this is physical block not partirion one
#define EXT2_BLOCK_SIZE(s_log_block_size) (1024 << s_log_block_size)
#define EXT2_BLOCK_GROUP_DESCRIPTOR_BLOCK_OFFSET 1

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
/**** REVISION NUMBERS ****/
#define EXT2_GOOD_OLD_REV 0
#define EXT2_DYNAMIC_REV 1

/**** EXT2_GOOD_OLD_REV SPECIFIC ****/
#define EXT2_GOOD_OLD_INODE_SIZE 128
#define EXT2_GOOD_OLD_FIRST_INO 11

#define EXT2_NO_FEATURES 0
/**** EXT2 COMPATIBLE FEATURES  ****/
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC    0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES   0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL     0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR        0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INO      0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX       0x0020

#define MEXT2_FEATURES_COMPAT (EXT2_NO_FEATURES)

/**** EXT2 INCOMPATIBLE FEATURES  ****/
#define EXT2_FEATURE_INCOMPAT_COMPRESSION   0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE      0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER       0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV   0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG       0x0010

#define MEXT2_FEATURES_INCOMPAT (EXT2_FEATURE_INCOMPAT_FILETYPE)

/**** EXT2 COMPATIBLE READ-ONLY FEATURES ****/
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004

#define MEXT2_FEATURES_RO_COMPAT (EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT2_FEATURE_RO_COMPAT_LARGE_FILE)


struct mext2_ext2_superblock;
struct mext2_sd;

enum ext2_errors
{
    EXT2_NO_ERRORS = 0,
    EXT2_INVALID_PATH,
    EXT2_READ_ERROR
};

struct mext2_inode_address
{
    uint32_t block_address;
    uint32_t block_offset;
    uint32_t block_inode_bitmap_address;
    uint32_t inode_local_index;
};

extern uint8_t ext2_errno;

uint8_t mext2_ext2_sd_parser(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock);
uint32_t mext2_inode_no_lookup(struct mext2_sd* sd, char* path); // path needs to start with '/'
uint32_t mext2_inode_no_lookup_from_dir_inode(struct mext2_sd* sd, uint32_t dir_inode_no, char* name); // name must not contain slashes
struct mext2_inode_address mext2_inode_no_to_addr(struct mext2_sd* sd, uint32_t inode_no);

#endif
