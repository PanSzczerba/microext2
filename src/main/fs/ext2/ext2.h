#ifndef MEXT2_FS_EXT2_H
#define MEXT2_FS_EXT2_H

#include <stdint.h>
#include "inode.h"
#include "common.h"

#define EXT2_BLOCK_SIZE(s_log_block_size) (1024 << s_log_block_size)
#define EXT2_BLOCK_GROUP_DESCRIPTOR_FST_BLOCK_OFFSET 1

#define EXT2_INVALID_BLOCK_NO 0

struct mext2_ext2_superblock;
struct mext2_sd;
struct block512_t;

uint8_t mext2_ext2_sd_parser(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock);

/* inode functions */
struct mext2_ext2_inode_address mext2_get_ext2_inode_address(struct mext2_sd* sd, uint32_t inode_no);
struct mext2_ext2_inode* mext2_get_ext2_inode(struct mext2_sd* sd, struct mext2_ext2_inode_address address);
struct mext2_ext2_inode* mext2_get_ext2_inode_by_no(struct mext2_sd* sd, uint32_t inode_no);
uint8_t mext2_put_ext2_inode(struct mext2_sd* sd, struct mext2_ext2_inode* inode, struct mext2_ext2_inode_address address); /* inode must be in usefull blocks along with corresponding block" */
uint32_t mext2_get_regular_file_inode(struct mext2_sd* sd, char* path, mext2_bool create_not_found); // path needs to start with '/'
uint32_t mext2_inode_no_lookup_from_dir_inode(struct mext2_sd* sd, uint32_t dir_inode_no, char* name); // name must not contain slashes
uint8_t mext2_ext2_inode_truncate(struct mext2_sd* sd, uint32_t inode_no, uint64_t new_size);
uint8_t mext2_ext2_inode_extend(struct mext2_sd* sd, struct mext2_ext2_inode_address inode_address, uint64_t new_size);

/* block functions */
struct block512_t* mext2_get_ext2_block(struct mext2_sd* sd, uint32_t block_no);
uint8_t mext2_put_ext2_block(struct mext2_sd* sd, block512_t* block, uint32_t block_no);
uint32_t mext2_get_data_block_by_inode_address_index(struct mext2_sd* sd, struct mext2_ext2_inode_address inode_address, uint32_t block_index);
uint32_t mext2_get_data_block_by_inode_index(struct mext2_sd* sd, struct mext2_ext2_inode* inode, uint32_t block_index);
uint16_t mext2_allocate_blocks(struct mext2_sd* sd, uint16_t primary_block_group_no, uint32_t* block_list, uint16_t blocks_to_allocate);
uint8_t mext2_deallocate_blocks(struct mext2_sd* sd, uint32_t* block_list, uint16_t block_list_size);

/* superblock get/set */
struct mext2_ext2_superblock* mext2_get_main_ext2_superblock(struct mext2_sd* sd);
uint8_t mext2_update_ext2_main_superblock_with_ptr(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock);
uint8_t mext2_update_ext2_main_superblock(struct mext2_sd* sd);
uint8_t mext2_update_ext2_superblocks(struct mext2_sd* sd);
uint8_t mext2_update_ext2_superblocks_with_ptr(struct mext2_sd* sd, struct mext2_ext2_superblock* superblock);
uint8_t mext2_update_ext2_block_group_descriptors(struct mext2_sd* sd);

#endif
