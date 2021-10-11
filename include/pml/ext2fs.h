/* ext2fs.h -- This file is part of PML.
   Copyright (C) 2021 XNSC

   PML is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   PML is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with PML. If not, see <https://www.gnu.org/licenses/>. */

#ifndef __PML_EXT2FS_H
#define __PML_EXT2FS_H

/*!
 * @file
 * @brief @c The second extended filesystem
 */

#include <pml/device.h>
#include <pml/vfs.h>

/*! The root inode of an ext2 filesystem */
#define EXT2_ROOT_INO           2

/*! Magic number of ext2 filesystems, must be in @ref ext2_super.s_magic */
#define EXT2_MAGIC              0xef53

/*! Offset of the primary superblock from the start of the partition */
#define EXT2_SUPER_OFFSET       1024

/* Superblock feature flags */

#define EXT2_FT_COMPAT_DIR_PREALLOC     0x0001
#define EXT2_FT_COMPAT_IMAGIC_INODES    0x0002
#define EXT3_FT_COMPAT_HAS_JOURNAL      0x0004
#define EXT2_FT_COMPAT_EXT_XATTR        0x0008
#define EXT2_FT_COMPAT_RESIZE_INODE     0x0010
#define EXT2_FT_COMPAT_DIR_INDEX        0x0020
#define EXT2_FT_COMPAT_LAZY_BG          0x0040
#define EXT2_FT_COMPAT_EXCLUDE_BITMAP   0x0100
#define EXT4_FT_COMPAT_SPARSE_SUPER2    0x0200
#define EXT4_FT_COMPAT_FAST_COMMIT      0x0400
#define EXT4_FT_COMPAT_STABLE_INODES    0x0800

#define EXT2_FT_INCOMPAT_COMPRESSION    0x0001
#define EXT2_FT_INCOMPAT_FILETYPE       0x0002
#define EXT3_FT_INCOMPAT_RECOVER        0x0004
#define EXT3_FT_INCOMPAT_JOURNAL_DEV    0x0008
#define EXT2_FT_INCOMPAT_META_BG        0x0010
#define EXT3_FT_INCOMPAT_EXTENTS        0x0040
#define EXT4_FT_INCOMPAT_64BIT          0x0080
#define EXT4_FT_INCOMPAT_MMP            0x0100
#define EXT4_FT_INCOMPAT_FLEX_BG        0x0200
#define EXT4_FT_INCOMPAT_EA_INODE       0x0400
#define EXT4_FT_INCOMPAT_DIRDATA        0x1000
#define EXT4_FT_INCOMPAT_CSUM_SEED      0x2000
#define EXT4_FT_INCOMPAT_LARGEDIR       0x4000
#define EXT4_FT_INCOMPAT_INLINE_DATA    0x8000
#define EXT4_FT_INCOMPAT_ENCRYPT        0x10000
#define EXT4_FT_INCOMPAT_CASEFOLD       0x20000

#define EXT2_FT_RO_COMPAT_SPARSE_SUPER  0x0001
#define EXT2_FT_RO_COMPAT_LARGE_FILE    0x0002
#define EXT4_FT_RO_COMPAT_HUGE_FILE     0x0008
#define EXT4_FT_RO_COMPAT_GDT_CSUM      0x0010
#define EXT4_FT_RO_COMPAT_DIR_NLINK     0x0020
#define EXT4_FT_RO_COMPAT_EXTRA_ISIZE   0x0040
#define EXT4_FT_RO_COMPAT_HAS_SNAPSHOT  0x0080
#define EXT4_FT_RO_COMPAT_QUOTA         0x0100
#define EXT4_FT_RO_COMPAT_BIGALLOC      0x0200
#define EXT4_FT_RO_COMPAT_METADATA_CSUM 0x0400
#define EXT4_FT_RO_COMPAT_REPLICA       0x0800
#define EXT4_FT_RO_COMPAT_READONLY      0x1000
#define EXT4_FT_RO_COMPAT_PROJECT       0x2000
#define EXT4_FT_RO_COMPAT_SHARED_BLOCKS 0x4000
#define EXT4_FT_RO_COMPAT_VERITY        0x8000

/*! Supported ext2 incompatible features */
#define EXT2_INCOMPAT_SUPPORT EXT2_FT_INCOMPAT_FILETYPE

/*! Supported ext2 read-only features */
#define EXT2_RO_COMPAT_SUPPORTED 0

/* Inode flags */

#define EXT2_SECRM_FL            0x00000001
#define EXT2_UNRM_FL             0x00000002
#define EXT2_COMPR_FL            0x00000004
#define EXT2_SYNC_FL             0x00000008
#define EXT2_IMMUTABLE_FL        0x00000010
#define EXT2_APPEND_FL           0x00000020
#define EXT2_NODUMP_FL           0x00000040
#define EXT4_NOATIME_FL          0x00000080
#define EXT2_DIRTY_FL            0x00000100
#define EXT2_COMPRBLK_FL         0x00000200
#define EXT2_NOCOMPR_FL          0x00000400
#define EXT4_ENCRYPT_FL          0x00000800
#define EXT2_BTREE_FL            0x00001000
#define EXT2_INDEX_FL            0x00001000
#define EXT2_IMAGIC_FL           0x00002000
#define EXT3_JOURNAL_DATA_FL     0x00004000
#define EXT2_NOTAIL_FL           0x00008000
#define EXT2_DIRSYNC_FL          0x00010000
#define EXT2_TOPDIR_FL           0x00020000
#define EXT4_HUGE_FILE_FL        0x00040000
#define EXT4_EXTENTS_FL          0x00080000
#define EXT4_VERITY_FL           0x00100000
#define EXT4_EA_INODE_FL         0x00200000
#define EXT4_SNAPFILE_FL         0x01000000
#define EXT4_SNAPFILE_DELETED_FL 0x04000000
#define EXT4_SNAPFILE_SHRUNK_FL  0x08000000
#define EXT4_INLINE_DATA_FL      0x10000000
#define EXT4_PROJINHERIT_FL      0x20000000
#define EXT4_CASEFOLD_FL         0x40000000
#define EXT2_RESERVED_FL         0x80000000

/* Block group descriptor flags */

#define EXT2_BG_INODE_UNUSED     0x0001
#define EXT2_BG_BLOCK_UNUSED     0x0002
#define EXT2_BG_INODE_ZEROED     0x0004

/* Other macros */

/*!
 * Major version of ext2 dynamic revision. Filesystems with a major version
 * of at least this value have more fields in the superblock and more features.
 * Filesystems with a lower major version number are known as `old' ext2
 * filesystems.
 */

#define EXT2_DYNAMIC             1

/*! First non-reserved inode on old ext2 filesystems */
#define EXT2_FIRST_INODE         11

/*! Number of direct blocks in an inode */
#define EXT2_NDIR_BLOCKS         12
/*! Index of the indirect block pointer in an inode */
#define EXT2_IND_BLOCK           EXT2_NDIR_BLOCKS
/*! Index of the doubly indirect block pointer in an inode */
#define EXT2_DIND_BLOCK          (EXT2_IND_BLOCK + 1)
/*! Index of the triply indirect block pointer in an inode */
#define EXT2_TIND_BLOCK          (EXT2_DIND_BLOCK + 1)
/*! Total number of block pointers in an inode */
#define EXT2_N_BLOCKS            (EXT2_TIND_BLOCK + 1)

/*!
 * Format of the superblock for ext2 filesystems.
 */

struct ext2_super
{
  uint32_t s_inodes_count;
  uint32_t s_blocks_count;
  uint32_t s_r_blocks_count;
  uint32_t s_free_blocks_count;
  uint32_t s_free_inodes_count;
  uint32_t s_first_data_block;
  uint32_t s_log_block_size;
  uint32_t s_log_cluster_size;
  uint32_t s_blocks_per_group;
  uint32_t s_clusters_per_group;
  uint32_t s_inodes_per_group;
  uint32_t s_mtime;
  uint32_t s_wtime;
  uint16_t s_mnt_count;
  int16_t s_max_mnt_count;
  uint16_t s_magic;
  uint16_t s_state;
  uint16_t s_errors;
  uint16_t s_minor_rev_level;
  uint32_t s_lastcheck;
  uint32_t s_checkinterval;
  uint32_t s_creator_os;
  uint32_t s_rev_level;
  uint16_t s_def_resuid;
  uint16_t s_def_resgid;
  uint32_t s_first_ino;
  uint16_t s_inode_size;
  uint16_t s_block_group_nr;
  uint32_t s_feature_compat;
  uint32_t s_feature_incompat;
  uint32_t s_feature_ro_compat;
  unsigned char s_uuid[16];
  unsigned char s_volume_name[16];
  unsigned char s_last_mounted[64];
  uint32_t s_algorithm_usage_bitmap;
  unsigned char s_prealloc_blocks;
  unsigned char s_prealloc_dir_blocks;
  uint16_t s_reserved_gdt_blocks;
  unsigned char s_journal_uuid[16];
  uint32_t s_journal_inum;
  uint32_t s_journal_dev;
  uint32_t s_last_orphan;
  uint32_t s_hash_seed[4];
  unsigned char s_def_hash_version;
  unsigned char s_jnl_backup_type;
  uint16_t s_desc_size;
  uint32_t s_default_mount_opts;
  uint32_t s_first_meta_bg;
  uint32_t s_mkfs_time;
  uint32_t s_jnl_blocks[17];
  uint32_t s_blocks_count_hi;
  uint32_t s_r_blocks_count_hi;
  uint32_t s_free_blocks_hi;
  uint16_t s_min_extra_isize;
  uint16_t s_want_extra_isize;
  uint32_t s_flags;
  uint16_t s_raid_stride;
  uint16_t s_mmp_update_interval;
  uint64_t s_mmp_block;
  uint32_t s_raid_stripe_width;
  unsigned char s_log_groups_per_flex;
  unsigned char s_checksum_type;
  unsigned char s_encryption_level;
  unsigned char s_reserved_pad;
  uint64_t s_kbytes_written;
  uint32_t s_snapshot_inum;
  uint32_t s_snapshot_id;
  uint64_t s_snapshot_r_blocks_count;
  uint32_t s_snapshot_list;
  uint32_t s_error_count;
  uint32_t s_first_error_time;
  uint32_t s_first_error_ino;
  uint64_t s_first_error_block;
  unsigned char s_first_error_func[32];
  uint32_t s_first_error_line;
  uint32_t s_last_error_time;
  uint32_t s_last_error_ino;
  uint32_t s_last_error_line;
  uint64_t s_last_error_block;
  unsigned char s_last_error_func[32];
  unsigned char s_mount_opts[64];
  uint32_t s_usr_quota_inum;
  uint32_t s_grp_quota_inum;
  uint32_t s_overhead_clusters;
  uint32_t s_backup_bgs[2];
  unsigned char s_encrypt_algos[4];
  unsigned char s_encrypt_pw_salt[16];
  uint32_t s_lpf_ino;
  uint32_t s_prj_quota_inum;
  uint32_t s_checksum_seed;
  unsigned char s_wtime_hi;
  unsigned char s_mtime_hi;
  unsigned char s_mkfs_time_hi;
  unsigned char s_lastcheck_hi;
  unsigned char s_first_error_time_hi;
  unsigned char s_last_error_time_hi;
  unsigned char s_first_error_errcode;
  unsigned char s_last_error_errcode;
  uint16_t s_encoding;
  uint16_t s_encoding_flags;
  unsigned char s_reserved[380];
  uint32_t s_checksum;
};

/*!
 * Structure of an ext2 inode.
 */

struct ext2_inode
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
  union
  {
    struct
    {
      uint32_t l_i_version;
    } linux1;
    struct
    {
      uint32_t h_i_translator;
    } hurd1;
  } osd1;
  uint32_t i_block[EXT2_N_BLOCKS];
  uint32_t i_generation;
  uint32_t i_file_acl;
  uint32_t i_size_high;
  uint32_t i_faddr;
  union
  {
    struct
    {
      uint16_t l_i_blocks_hi;
      uint16_t l_i_file_acl_hi;
      uint16_t l_i_uid_high;
      uint16_t l_i_gid_high;
      uint16_t l_i_checksum_lo;
#define i_checksum_lo osd2.linux2.l_i_checksum_lo
      uint16_t l_i_reserved;
    } linux2;
    struct
    {
      unsigned char h_i_frag;
      unsigned char h_i_fsize;
      uint16_t h_i_mode_high;
      uint16_t h_i_uid_high;
      uint16_t h_i_gid_high;
      uint32_t h_i_author;
    } hurd2;
  } osd2;
};

/*!
 * Format of a block group descriptor.
 */

struct ext2_group_desc
{
  uint32_t bg_block_bitmap;
  uint32_t bg_inode_bitmap;
  uint32_t bg_inode_table;
  uint16_t bg_free_blocks_count;
  uint16_t bg_free_inodes_count;
  uint16_t bg_used_dirs_count;
  uint16_t bg_flags;
  uint32_t bg_exclude_bitmap_lo;
  uint16_t bg_block_bitmap_csum_lo;
  uint16_t bg_inode_bitmap_csum_lo;
  uint16_t bg_itable_unused;
  uint16_t bg_checksum;
};

/*!
 * Format of a block group descriptor for 64-bit ext4 filesystems.
 */

struct ext4_group_desc
{
  uint32_t bg_block_bitmap;
  uint32_t bg_inode_bitmap;
  uint32_t bg_inode_table;
  uint16_t bg_free_blocks_count;
  uint16_t bg_free_inodes_count;
  uint16_t bg_used_dirs_count;
  uint16_t bg_flags;
  uint32_t bg_exclude_bitmap_lo;
  uint16_t bg_block_bitmap_csum_lo;
  uint16_t bg_inode_bitmap_csum_lo;
  uint16_t bg_itable_unused;
  uint16_t bg_checksum;
  uint32_t bg_block_bitmap_hi;
  uint32_t bg_inode_bitmap_hi;
  uint32_t bg_inode_table_hi;
  uint16_t bg_free_blocks_count_hi;
  uint16_t bg_free_inodes_count_hi;
  uint16_t bg_used_dirs_count_hi;
  uint16_t bg_itable_unused_hi;
  uint32_t bg_exclude_bitmap_hi;
  uint16_t bg_block_bitmap_csum_hi;
  uint16_t bg_inode_bitmap_csum_hi;
  uint32_t bg_reserved;
};

/*!
 * Structure containing information about an instance of an ext2 filesystem.
 * This structure is used as the @ref mount.data field of a mount structure.
 */

struct ext2_fs
{
  struct ext2_super super;              /*!< Copy of superblock */
  struct block_device *device;          /*!< Block device of filesystem */
  blksize_t block_size;                 /*!< Size of a block */
  size_t inode_size;                    /*!< Size of an inode */
  struct ext2_group_desc *group_descs;  /*!< Array of block group descriptors */
  size_t group_desc_count;              /*!< Block group descriptor count */
};

/*!
 * Determines the number of block group descriptors in an ext2 filesystem.
 *
 * @param super the superblock of the filesystem
 * @return number of block group descriptors
 */

static inline size_t
ext2_group_desc_count (struct ext2_super *super)
{
  size_t by_block = (super->s_blocks_count + super->s_blocks_per_group - 1) /
    super->s_blocks_per_group;
  size_t by_inode = (super->s_inodes_count + super->s_inodes_per_group - 1) /
    super->s_inodes_per_group;
  return by_block > by_inode ? by_block : by_inode;
}

__BEGIN_DECLS

extern const struct mount_ops ext2_mount_ops;
extern const struct vnode_ops ext2_vnode_ops;

int ext2_mount (struct mount *mp, unsigned int flags);
int ext2_unmount (struct mount *mp, unsigned int flags);
int ext2_check (struct vnode *vp);

int ext2_lookup (struct vnode **result, struct vnode *dir, const char *name);
ssize_t ext2_read (struct vnode *vp, void *buffer, size_t len, off_t offset);
ssize_t ext2_write (struct vnode *vp, const void *buffer, size_t len,
		    off_t offset);
void ext2_sync (struct vnode *vp);
int ext2_create (struct vnode **result, struct vnode *dir, const char *name,
		 mode_t mode, dev_t rdev);
int ext2_mkdir (struct vnode **result, struct vnode *dir, const char *name,
		mode_t mode);
int ext2_rename (struct vnode *vp, struct vnode *dir, const char *name);
int ext2_link (struct vnode *dir, struct vnode *vp, const char *name);
int ext2_symlink (struct vnode *dir, const char *name, const char *target);
off_t ext2_readdir (struct vnode *dir, struct pml_dirent *dirent, off_t offset);
ssize_t ext2_readlink (struct vnode *vp, char *buffer, size_t len);
int ext2_bmap (struct vnode *vp, block_t *result, block_t block, size_t num);
int ext2_fill (struct vnode *vp);

__END_DECLS

#endif