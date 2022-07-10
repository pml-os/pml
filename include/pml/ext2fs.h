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

/* Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Library
 * General Public License, version 2.
 * %End-Header%
 */

#ifndef __PML_EXT2FS_H
#define __PML_EXT2FS_H

/*!
 * @file
 * @brief The second extended filesystem
 */

#include <pml/device.h>
#include <pml/vfs.h>

/*! File types */

enum ext2_file_type
{
  EXT2_FILE_UNKNOWN,            /*!< Unknown file type or not supported */
  EXT2_FILE_REG,                /*!< Regular file */
  EXT2_FILE_DIR,                /*!< Directory */
  EXT2_FILE_CHR,                /*!< Character device */
  EXT2_FILE_BLK,                /*!< Block device */
  EXT2_FILE_FIFO,               /*!< Named pipe */
  EXT2_FILE_SOCK,               /*!< Socket */
  EXT2_FILE_LNK                 /*!< Symbolic link */
};

/*! Operating systems */

enum ext2_os_type
{
  EXT2_OS_LINUX,
  EXT2_OS_HURD,
  EXT2_OS_MASIX,
  EXT2_OS_FREEBSD,
  EXT2_OS_LITES
};

/* Filesystem state */

#define EXT2_STATE_VALID                (1 << 0)
#define EXT2_STATE_ERROR                (1 << 1)
#define EXT3_STATE_ORPHANS              (1 << 2)
#define EXT4_STATE_FC_REPLAY            (1 << 5)

/* Superblock feature flags */

#define EXT2_FT_COMPAT_DIR_PREALLOC     (1 << 0)
#define EXT2_FT_COMPAT_IMAGIC_INODES    (1 << 1)
#define EXT3_FT_COMPAT_HAS_JOURNAL      (1 << 2)
#define EXT2_FT_COMPAT_EXT_XATTR        (1 << 3)
#define EXT2_FT_COMPAT_RESIZE_INODE     (1 << 4)
#define EXT2_FT_COMPAT_DIR_INDEX        (1 << 5)
#define EXT2_FT_COMPAT_LAZY_BG          (1 << 6)
#define EXT2_FT_COMPAT_EXCLUDE_BITMAP   (1 << 8)
#define EXT4_FT_COMPAT_SPARSE_SUPER2    (1 << 9)
#define EXT4_FT_COMPAT_FAST_COMMIT      (1 << 10)
#define EXT4_FT_COMPAT_STABLE_INODES    (1 << 11)

#define EXT2_FT_INCOMPAT_COMPRESSION    (1 << 0)
#define EXT2_FT_INCOMPAT_FILETYPE       (1 << 1)
#define EXT3_FT_INCOMPAT_RECOVER        (1 << 2)
#define EXT3_FT_INCOMPAT_JOURNAL_DEV    (1 << 3)
#define EXT2_FT_INCOMPAT_META_BG        (1 << 4)
#define EXT3_FT_INCOMPAT_EXTENTS        (1 << 6)
#define EXT4_FT_INCOMPAT_64BIT          (1 << 7)
#define EXT4_FT_INCOMPAT_MMP            (1 << 8)
#define EXT4_FT_INCOMPAT_FLEX_BG        (1 << 9)
#define EXT4_FT_INCOMPAT_EA_INODE       (1 << 10)
#define EXT4_FT_INCOMPAT_DIRDATA        (1 << 12)
#define EXT4_FT_INCOMPAT_CSUM_SEED      (1 << 13)
#define EXT4_FT_INCOMPAT_LARGEDIR       (1 << 14)
#define EXT4_FT_INCOMPAT_INLINE_DATA    (1 << 15)
#define EXT4_FT_INCOMPAT_ENCRYPT        (1 << 16)
#define EXT4_FT_INCOMPAT_CASEFOLD       (1 << 17)

#define EXT2_FT_RO_COMPAT_SPARSE_SUPER  (1 << 0)
#define EXT2_FT_RO_COMPAT_LARGE_FILE    (1 << 1)
#define EXT4_FT_RO_COMPAT_HUGE_FILE     (1 << 3)
#define EXT4_FT_RO_COMPAT_GDT_CSUM      (1 << 4)
#define EXT4_FT_RO_COMPAT_DIR_NLINK     (1 << 5)
#define EXT4_FT_RO_COMPAT_EXTRA_ISIZE   (1 << 6)
#define EXT4_FT_RO_COMPAT_HAS_SNAPSHOT  (1 << 7)
#define EXT4_FT_RO_COMPAT_QUOTA         (1 << 8)
#define EXT4_FT_RO_COMPAT_BIGALLOC      (1 << 9)
#define EXT4_FT_RO_COMPAT_METADATA_CSUM (1 << 10)
#define EXT4_FT_RO_COMPAT_REPLICA       (1 << 11)
#define EXT4_FT_RO_COMPAT_READONLY      (1 << 12)
#define EXT4_FT_RO_COMPAT_PROJECT       (1 << 13)
#define EXT4_FT_RO_COMPAT_SHARED_BLOCKS (1 << 14)
#define EXT4_FT_RO_COMPAT_VERITY        (1 << 15)

/*! Supported ext2 incompatible features */
#define EXT2_INCOMPAT_SUPPORT (EXT2_FT_INCOMPAT_FILETYPE	\
			       | EXT2_FT_INCOMPAT_META_BG	\
			       | EXT3_FT_INCOMPAT_RECOVER	\
			       | EXT3_FT_INCOMPAT_EXTENTS	\
			       | EXT4_FT_INCOMPAT_FLEX_BG	\
			       | EXT4_FT_INCOMPAT_EA_INODE	\
			       | EXT4_FT_INCOMPAT_MMP		\
			       | EXT4_FT_INCOMPAT_64BIT		\
			       | EXT4_FT_INCOMPAT_INLINE_DATA	\
			       | EXT4_FT_INCOMPAT_ENCRYPT	\
			       | EXT4_FT_INCOMPAT_CASEFOLD	\
			       | EXT4_FT_INCOMPAT_CSUM_SEED	\
			       | EXT4_FT_INCOMPAT_LARGEDIR)

/*! Supported ext2 read-only features */
#define EXT2_RO_COMPAT_SUPPORT (EXT2_FT_RO_COMPAT_SPARSE_SUPER		\
				| EXT4_FT_RO_COMPAT_HUGE_FILE		\
				| EXT2_FT_RO_COMPAT_LARGE_FILE		\
				| EXT4_FT_RO_COMPAT_DIR_NLINK		\
				| EXT4_FT_RO_COMPAT_EXTRA_ISIZE		\
				| EXT4_FT_RO_COMPAT_GDT_CSUM		\
				| EXT4_FT_RO_COMPAT_BIGALLOC		\
				| EXT4_FT_RO_COMPAT_QUOTA		\
				| EXT4_FT_RO_COMPAT_METADATA_CSUM	\
				| EXT4_FT_RO_COMPAT_READONLY		\
				| EXT4_FT_RO_COMPAT_PROJECT		\
				| EXT4_FT_RO_COMPAT_SHARED_BLOCKS	\
				| EXT4_FT_RO_COMPAT_VERITY)

/* Inode flags */

#define EXT2_SECRM_FL                   (1 << 0)
#define EXT2_UNRM_FL                    (1 << 1)
#define EXT2_COMPR_FL                   (1 << 2)
#define EXT2_SYNC_FL                    (1 << 3)
#define EXT2_IMMUTABLE_FL               (1 << 4)
#define EXT2_APPEND_FL                  (1 << 5)
#define EXT2_NODUMP_FL                  (1 << 6)
#define EXT4_NOATIME_FL                 (1 << 7)
#define EXT2_DIRTY_FL                   (1 << 8)
#define EXT2_COMPRBLK_FL                (1 << 9)
#define EXT2_NOCOMPR_FL                 (1 << 10)
#define EXT4_ENCRYPT_FL                 (1 << 11)
#define EXT2_BTREE_FL                   (1 << 12)
#define EXT2_INDEX_FL                   (1 << 12)
#define EXT2_IMAGIC_FL                  (1 << 13)
#define EXT3_JOURNAL_DATA_FL            (1 << 14)
#define EXT2_NOTAIL_FL                  (1 << 15)
#define EXT2_DIRSYNC_FL                 (1 << 16)
#define EXT2_TOPDIR_FL                  (1 << 17)
#define EXT4_HUGE_FILE_FL               (1 << 18)
#define EXT4_EXTENTS_FL                 (1 << 19)
#define EXT4_VERITY_FL                  (1 << 20)
#define EXT4_EA_INODE_FL                (1 << 21)
#define EXT4_SNAPFILE_FL                (1 << 24)
#define EXT4_SNAPFILE_DELETED_FL        (1 << 26)
#define EXT4_SNAPFILE_SHRUNK_FL         (1 << 27)
#define EXT4_INLINE_DATA_FL             (1 << 28)
#define EXT4_PROJINHERIT_FL             (1 << 29)
#define EXT4_CASEFOLD_FL                (1 << 30)
#define EXT2_RESERVED_FL                (1 << 31)

/* Helper macros */

/*! Magic number of ext2 filesystems, must be in @ref ext2_super.s_magic */
#define EXT2_MAGIC                      0xef53

/*! The root inode of an ext2 filesystem */
#define EXT2_ROOT_INODE                 2

/*! Maximum allowed file name length */
#define EXT2_MAX_NAME                   255

/*! Offset of the primary superblock in bytes */
#define EXT2_SUPER_OFFSET               1024

#define EXT3_EXTENT_MAGIC               0xf30a
#define EXT2_DIR_NAME_CHECKSUM          0xde00

#define EXT2_FILE_BUFFER_VALID          (1 << 13)
#define EXT2_FILE_BUFFER_DIRTY          (1 << 14)

#define EXT2_OLD_REV                    0  /*!< Old revision of ext2 */
#define EXT2_DYNAMIC_REV                1  /*!< Ext2 with dynamic features */

/*! Fixed size of an inode on old revision filesystems */
#define EXT2_OLD_INODE_SIZE             128

/*! Fixed number of the first non-reserved inode on old revision filesystems */
#define EXT2_OLD_FIRST_INODE            11

#define EXT2_MIN_BLOCK_LOG_SIZE         10  /*!< 1024 byte blocks */
#define EXT2_MAX_BLOCK_LOG_SIZE         16  /*!< 65536 byte blocks */
#define EXT2_MIN_BLOCK_SIZE             (1 << EXT2_MIN_BLOCK_LOG_SIZE)
#define EXT2_MAX_BLOCK_SIZE             (1 << EXT2_MAX_BLOCK_LOG_SIZE)

#define EXT2_MIN_DESC_SIZE              32
#define EXT2_MIN_DESC_SIZE_64           64
#define EXT2_MAX_DESC_SIZE              EXT2_MIN_BLOCK_SIZE

#define EXT4_INODE_CSUM_HI_EXTRA_END					\
  (offsetof (struct ext2_large_inode, i_checksum_hi) + 2 - EXT2_OLD_INODE_SIZE)
#define EXT4_BG_BLOCK_BITMAP_CSUM_HI_END				\
  (offsetof (struct ext4_group_desc, bg_block_bitmap_csum_hi) + 2)
#define EXT4_BG_INODE_BITMAP_CSUM_HI_END				\
  (offsetof (struct ext4_group_desc, bg_inode_bitmap_csum_hi) + 2)

#define EXT2_DIR_ENTRY_HEADER_LEN       8
#define EXT2_DIR_ENTRY_HASH_LEN         8
#define EXT2_DIR_PAD                    4
#define EXT2_DIR_ROUND                  (EXT2_DIR_PAD - 1)

/* Mounted filesystem flags */

#define EXT2_FLAG_CHANGED               (1 << 0)
#define EXT2_FLAG_DIRTY                 (1 << 1)
#define EXT2_FLAG_VALID                 (1 << 2)
#define EXT2_FLAG_IB_DIRTY              (1 << 3)
#define EXT2_FLAG_BB_DIRTY              (1 << 4)
#define EXT2_FLAG_64BIT                 (1 << 5)

#define BMAP_ALLOC                      (1 << 0)
#define BMAP_SET                        (1 << 1)
#define BMAP_UNINIT                     (1 << 2)
#define BMAP_ZERO                       (1 << 3)
#define BMAP_RET_UNINIT                 1

/* Bitmap types */

#define EXT2_BITMAP_BLOCK               (1 << 0)    /*!< Block usage bitmap */
#define EXT2_BITMAP_INODE               (1 << 1)    /*!< Inode usage bitmap */

#define EXT2_BMAP_MAGIC_BLOCK           (1 << 0)
#define EXT2_BMAP_MAGIC_INODE           (1 << 1)
#define EXT2_BMAP_MAGIC_BLOCK64         (1 << 2)
#define EXT2_BMAP_MAGIC_INODE64         (1 << 3)
#define EXT2_BMAP_MAGIC_VALID(m) (m == EXT2_BMAP_MAGIC_BLOCK		\
				  || m == EXT2_BMAP_MAGIC_INODE		\
				  || m == EXT2_BMAP_MAGIC_BLOCK64	\
				  || m == EXT2_BMAP_MAGIC_INODE64)

#define EXT2_BG_INODE_UNINIT            (1 << 0)
#define EXT2_BG_BLOCK_UNINIT            (1 << 1)
#define EXT2_BG_BLOCK_ZEROED            (1 << 2)

#define BLOCK_CHANGED                   (1 << 0)
#define BLOCK_ABORT                     (1 << 1)
#define BLOCK_ERROR                     (1 << 2)
#define BLOCK_INLINE_CHANGED            (1 << 3)

#define BLOCK_FLAG_APPEND               (1 << 0)
#define BLOCK_FLAG_DEPTH_TRAVERSE       (1 << 1)
#define BLOCK_FLAG_DATA_ONLY            (1 << 2)
#define BLOCK_FLAG_READ_ONLY            (1 << 3)
#define BLOCK_FLAG_NO_LARGE             (1 << 4)

#define BLOCK_COUNT_IND                 (-1)
#define BLOCK_COUNT_DIND                (-2)
#define BLOCK_COUNT_TIND                (-3)
#define BLOCK_COUNT_TRANSLATOR          (-4)

#define BLOCK_ALLOC_UNKNOWN             0
#define BLOCK_ALLOC_DATA                1
#define BLOCK_ALLOC_METADATA            2

#define DIRENT_CHANGED                  1
#define DIRENT_ABORT                    2
#define DIRENT_ERROR                    3

#define DIRENT_FLAG_EMPTY               (1 << 0)
#define DIRENT_FLAG_REMOVED             (1 << 1)
#define DIRENT_FLAG_CHECKSUM            (1 << 2)
#define DIRENT_FLAG_INLINE              (1 << 3)

#define DIRENT_DOT_FILE                 1
#define DIRENT_DOT_DOT_FILE             2
#define DIRENT_OTHER_FILE               3
#define DIRENT_DELETED_FILE             4
#define DIRENT_CHECKSUM                 5

#define FLUSH_VALID                     1

#define EXT2_INIT_MAX_LEN               (1 << 15)
#define EXT2_UNINIT_MAX_LEN             (EXT2_INIT_MAX_LEN - 1)
#define EXT2_MAX_EXTENT_LBLK            (((block_t) 1 << 32) - 1)
#define EXT2_MAX_EXTENT_PBLK            (((block_t) 1 << 48) - 1)

#define EXT2_EXTENT_FLAGS_LEAF          (1 << 0)
#define EXT2_EXTENT_FLAGS_UNINIT        (1 << 1)
#define EXT2_EXTENT_FLAGS_SECOND_VISIT  (1 << 2)

#define EXT2_EXTENT_CURRENT             0x00
#define EXT2_EXTENT_ROOT                0x01
#define EXT2_EXTENT_LAST_LEAF           0x02
#define EXT2_EXTENT_FIRST_SIB           0x03
#define EXT2_EXTENT_LAST_SIB            0x04
#define EXT2_EXTENT_NEXT_SIB            0x05
#define EXT2_EXTENT_PREV_SIB            0x06
#define EXT2_EXTENT_NEXT_LEAF           0x07
#define EXT2_EXTENT_PREV_LEAF           0x08
#define EXT2_EXTENT_NEXT                0x09
#define EXT2_EXTENT_PREV                0x0a
#define EXT2_EXTENT_UP                  0x0b
#define EXT2_EXTENT_DOWN                0x0c
#define EXT2_EXTENT_DOWN_LAST           0x0d
#define EXT2_EXTENT_MOVE_MASK           0x0f

#define EXT2_EXTENT_INSERT_AFTER        1
#define EXT2_EXTENT_INSERT_NOSPLIT      2
#define EXT2_EXTENT_DELETE_KEEP_EMPTY   1
#define EXT2_EXTENT_SET_BMAP_UNINIT     1

/*! Number of direct block pointers per inode */
#define EXT2_NDIR_BLOCKS                12
/*! Index in @ref ext2_inode.i_block to the indirect block pointer */
#define EXT2_IND_BLOCK                  EXT2_NDIR_BLOCKS
/*! Index in @ref ext2_inode.i_block to the doubly indirect block pointer */
#define EXT2_DIND_BLOCK                 (EXT2_IND_BLOCK + 1)
/*! Index in @ref ext2_inode.i_block to the triply indirect block pointer */
#define EXT2_TIND_BLOCK                 (EXT2_DIND_BLOCK + 1)
/*! Size of the @ref ext2_inode.i_block array */
#define EXT2_N_BLOCKS                   (EXT2_TIND_BLOCK + 1)

#define EXT2_CRC32C_CHECKSUM            1

/* Helper macro functions */

#define EXT2_BLOCK_SIZE(s) (EXT2_MIN_BLOCK_SIZE << s.s_log_block_size)
#define EXT2_BLOCK_SIZE_BITS(s) (s.s_log_block_size + 10)
#define EXT2_CLUSTER_SIZE(s) (EXT2_MIN_BLOCK_SIZE << s.s_log_cluster_size)
#define EXT2_INODE_SIZE(s)						\
  (s.s_rev_level == EXT2_OLD_REV ? EXT2_OLD_INODE_SIZE : s.s_inode_size)
#define EXT2_DESC_SIZE(s) (s.s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ? \
			   s.s_desc_size : EXT2_MIN_DESC_SIZE)
#define EXT2_FIRST_INODE(s)						\
  (s.s_rev_level == EXT2_OLD_REV ? EXT2_OLD_FIRST_INODE : s.s_first_ino)
#define EXT2_INODES_PER_BLOCK(s) (EXT2_BLOCK_SIZE (s) / EXT2_INODE_SIZE (s))
#define EXT2_DESC_PER_BLOCK(s) (EXT2_BLOCK_SIZE (s) / EXT2_DESC_SIZE (s))
#define EXT2_MAX_BLOCKS_PER_GROUP(s)				\
  ((unsigned int) (65528 * (EXT2_CLUSTER_SIZE (s) / EXT2_BLOCK_SIZE (s))))
#define EXT2_MAX_INODES_PER_GROUP(s)			\
  ((unsigned int) (65536 - EXT2_INODES_PER_BLOCK (s)))
#define EXT2_CLUSTER_RATIO(fs) (1 << fs->cluster_ratio_bits)
#define EXT2_CLUSTER_MASK(fs) (EXT2_CLUSTER_RATIO (fs) - 1)
#define EXT2_GROUPS_TO_BLOCKS(s, g) ((block_t) s.s_blocks_per_group * (g))
#define EXT2_GROUPS_TO_CLUSTERS(s, g) ((block_t) s.s_clusters_per_group * (g))
#define EXT2_B2C(fs, block) ((block) >> (fs)->cluster_ratio_bits)
#define EXT2_C2B(fs, cluster) ((cluster) << (fs)->cluster_ratio_bits)
#define EXT2_NUM_B2C(fs, blocks)					\
  (((blocks) + EXT2_CLUSTER_MASK (fs)) >> (fs)->cluster_ratio_bits)
#define EXT2_I_SIZE(i) ((i).i_size | (uint64_t) (i).i_size_high << 32)
#define EXT2_MAX_STRIDE_LENGTH(s) (4194304 / EXT2_BLOCK_SIZE (s))
#define EXT2_FIRST_EXTENT(h)						\
  ((struct ext3_extent *) ((char *) (h) + sizeof (struct ext3_extent_header)))
#define EXT2_FIRST_INDEX(h)						\
  ((struct ext3_extent_index *) ((char *) (h) +				\
				 sizeof (struct ext3_extent_header)))
#define EXT2_HAS_FREE_INDEX(path)			\
  ((path)->header->eh_entries < (path)->header->eh_max)
#define EXT2_LAST_EXTENT(h) (EXT2_FIRST_EXTENT (h) + (h)->eh_entries - 1)
#define EXT2_LAST_INDEX(h) (EXT2_FIRST_INDEX (h) + (h)->eh_entries - 1)
#define EXT2_MAX_EXTENT(h) (EXT2_FIRST_EXTENT (h) + (h)->eh_max - 1)
#define EXT2_MAX_INDEX(h) (EXT2_FIRST_INDEX (h) + (h)->eh_max - 1)
#define EXT2_EXTENT_TAIL_OFFSET(h) (sizeof (struct ext3_extent_header) + \
				    sizeof (struct ext3_extent) * (h)->eh_max)
#define EXT2_EXTENT_TAIL(h)						\
  ((struct ext3_extent_tail *) ((char *) (h) + EXT2_EXTENT_TAIL_OFFSET (h)))
#define EXT2_DIRENT_TAIL(block, size)					\
  ((struct ext2_dirent_tail *) ((char *) (block) + (size) -		\
				sizeof (struct ext2_dirent_tail)))

/*! Type of a block group number */
typedef unsigned int ext2_bgrp_t;

/*! Type of an entry in an inode's indirect block */
typedef unsigned int ext2_block_t;

/*! Structure of the superblock */

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
  uint32_t s_reserved[95];
  uint32_t s_checksum;
};

/*! Structure of an on-disk inode. */

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

/*! Structure of a large inode */

struct ext2_large_inode
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
  uint16_t i_extra_isize;
  uint16_t i_checksum_hi;
  uint32_t i_ctime_extra;
  uint32_t i_mtime_extra;
  uint32_t i_atime_extra;
  uint32_t i_crtime;
  uint32_t i_crtime_extra;
  uint32_t i_version_hi;
  uint32_t i_projid;
};

/*! Structure of a block group descriptor. */

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

/*! Structure of a block group descriptor for 64-bit ext4 filesystems. */

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

struct ext2_dx_entry
{
  uint32_t hash;
  uint32_t block;
};

struct ext2_dx_countlimit
{
  uint16_t limit;
  uint16_t count;
};

struct ext2_dx_root_info
{
  uint32_t reserved_zero;
  unsigned char hash_version;
  unsigned char info_length;
  unsigned char indirect_levels;
  unsigned char unused_flags;
};

struct ext2_dx_tail
{
  uint32_t dt_reserved;
  uint32_t dt_checksum;
};

/*! Format of a directory entry for linked list directories. */

struct ext2_dirent
{
  uint32_t d_inode;
  uint16_t d_rec_len;
  uint16_t d_name_len;
  char d_name[EXT2_MAX_NAME];
};

struct ext2_dirent_tail
{
  uint32_t det_reserved_zero1;
  uint16_t det_rec_len;
  uint16_t det_reserved_name_len;
  uint32_t det_checksum;
};

struct ext4_mmp_block
{
  uint32_t mmp_magic;
  uint32_t mmp_seq;
  uint64_t mmp_time;
  unsigned char mmp_nodename[64];
  unsigned char mmp_bdevname[32];
  uint16_t mmp_check_interval;
  uint16_t mmp_pad1;
  uint32_t mmp_pad2[226];
  uint32_t mmp_checksum;
};

/*!
 * Private vnode data for an open file in an ext2 filesystem. This structure
 * is used as the @ref vnode.data field of a vnode structure.
 */

struct ext2_file
{
  struct ext2_inode inode;
  ino_t ino;
  uint64_t pos;
  block_t block;
  block_t physblock;
  int flags;
  char *buffer;
};

/*! Structure for saving information about a file lookup */

struct ext2_lookup_ctx
{
  const char *name;
  int namelen;
  ino_t *inode;
  int found;
};

struct ext2_inode_cache_entry
{
  ino_t ino;
  struct ext2_inode *inode;
};

struct ext2_inode_cache
{
  void *buffer;
  block_t block;
  int cache_last;
  unsigned int cache_size;
  int refcnt;
  struct ext2_inode_cache_entry *cache;
};

/*! Ext2 64-bit bitmap types */

enum ext2_bitmap_type
{
  EXT2_BMAP64_BITARRAY,
  EXT2_BMAP64_RBTREE,
  EXT2_BMAP64_AUTODIR
};

struct ext2_bitmap
{
  int magic;
};

struct ext2_bitmap32
{
  int magic;
  uint32_t start;
  uint32_t end;
  uint32_t real_end;
  char *bitmap;
};

struct ext2_bitmap64;
struct ext2_fs;

struct ext2_bitmap_ops
{
  enum ext2_bitmap_type type;
  int (*new_bmap) (struct ext2_fs *, struct ext2_bitmap64 *);
  void (*free_bmap) (struct ext2_bitmap64 *);
  void (*mark_bmap) (struct ext2_bitmap64 *, uint64_t);
  void (*unmark_bmap) (struct ext2_bitmap64 *, uint64_t);
  int (*test_bmap) (struct ext2_bitmap64 *, uint64_t);
  void (*mark_bmap_extent) (struct ext2_bitmap64 *, uint64_t, unsigned int);
  void (*unmark_bmap_extent) (struct ext2_bitmap64 *, uint64_t, unsigned int);
  void (*set_bmap_range) (struct ext2_bitmap64 *, uint64_t, size_t, void *);
  void (*get_bmap_range) (struct ext2_bitmap64 *, uint64_t, size_t, void *);
  int (*find_first_zero) (struct ext2_bitmap64 *, uint64_t, uint64_t,
			  uint64_t *);
  int (*find_first_set) (struct ext2_bitmap64 *, uint64_t, uint64_t,
			 uint64_t *);
};

struct ext2_bitmap64
{
  int magic;
  struct ext2_bitmap_ops *ops;
  int flags;
  uint64_t start;
  uint64_t end;
  uint64_t real_end;
  int cluster_bits;
  void *private;
};

#define EXT2_BITMAP_IS_32(b) (b->magic == EXT2_BMAP_MAGIC_BLOCK		\
			      || b->magic == EXT2_BMAP_MAGIC_INODE)
#define EXT2_BITMAP_IS_64(b) (b->magic == EXT2_BMAP_MAGIC_BLOCK64	\
			      || b->magic == EXT2_BMAP_MAGIC_INODE64)

/*!
 * Structure containing information about an instance of an ext2 filesystem.
 * This structure is used as the @ref mount.data field of a mount structure.
 */

struct ext2_fs
{
  struct ext2_super super;              /*!< Copy of superblock */
  struct block_device *device;          /*!< Device containing filesystem */
  unsigned int mflags;                  /*!< Mount flags */
  int flags;                            /*!< Driver-specific flags */
  blksize_t blksize;                    /*!< Block size */
  unsigned int group_desc_count;        /*!< Number of block groups */
  unsigned long desc_blocks;
  struct ext2_group_desc *group_desc;
  unsigned int inode_blocks_per_group;
  struct ext2_bitmap *block_bitmap;
  struct ext2_bitmap *inode_bitmap;
  uint32_t stride;
  int cluster_ratio_bits;
  uint16_t default_bitmap_type;
  struct ext2_inode_cache *icache;
  void *mmp_buffer;
  int mmp_fd;
  time_t mmp_last_written;
  uint32_t checksum_seed;
};

struct ext3_extent_header
{
  uint16_t eh_magic;
  uint16_t eh_entries;
  uint16_t eh_max;
  uint16_t eh_depth;
  uint32_t eh_generation;
};

struct ext3_extent_index
{
  uint32_t ei_block;
  uint32_t ei_leaf;
  uint16_t ei_leaf_hi;
  uint16_t ei_unused;
};

struct ext3_extent_info
{
  int curr_entry;
  int curr_level;
  int num_entries;
  int max_entries;
  int max_depth;
  int bytes_avail;
  block_t max_lblk;
  block_t max_pblk;
  uint32_t max_len;
  uint32_t max_uninit_len;
};

struct ext3_extent_tail
{
  uint32_t et_checksum;
};

struct ext3_extent
{
  uint32_t ee_block;
  uint16_t ee_len;
  uint16_t ee_start_hi;
  uint32_t ee_start;
};

struct ext3_extent_path
{
  uint32_t block;
  uint16_t depth;
  struct ext3_extent *extent;
  struct ext3_extent_index *index;
  struct ext3_extent_header *header;
  void *bh;
};

struct ext3_generic_extent
{
  block_t e_pblk;
  block_t e_lblk;
  uint32_t e_len;
  uint32_t e_flags;
};

struct ext3_generic_extent_path
{
  char *buffer;
  int entries;
  int max_entries;
  int left;
  int visit_num;
  int flags;
  block_t end_block;
  void *curr;
};

struct ext3_extent_handle
{
  struct ext2_fs *fs;
  ino_t ino;
  struct ext2_inode *inode;
  struct ext2_inode inode_buf;
  int type;
  int level;
  int max_depth;
  int max_paths;
  struct ext3_generic_extent_path *path;
};

/*! Structure used to store information about a block allocation operation */

struct ext2_balloc_ctx
{
  ino_t ino;
  struct ext2_inode *inode;
  block_t block;
  int flags;
};

/*!
 * Structure used to store information when iterating through the blocks
 * of an inode
 */

struct ext2_block_ctx
{
  struct ext2_fs *fs;
  int (*func) (struct ext2_fs *, block_t *, blkcnt_t, block_t, int, void *);
  blkcnt_t blkcnt;
  int flags;
  int err;
  char *ind_buf;
  char *dind_buf;
  char *tind_buf;
  void *private;
};

/*! Structure used to store information when iterating through a directory */

struct ext2_dir_ctx
{
  struct vnode *dir;
  int flags;
  char *buffer;
  size_t bufsize;
  int (*func) (struct vnode *, int, struct ext2_dirent *, int, blksize_t,
	       char *, void *);
  void *private;
  int err;
};

/*! Structure used to store information when creating a link */

struct ext2_link_ctx
{
  struct ext2_fs *fs;
  const char *name;
  size_t namelen;
  ino_t inode;
  int flags;
  int done;
  int err;
  struct ext2_dirent *prev;
};

/*! Structure used to store information during a directory expand operation */

struct ext2_dir_expand_ctx
{
  int done;
  int newblocks;
  block_t goal;
  int err;
  struct vnode *dir;
};

/*! Structure used to store information while reading a directory */

struct ext2_readdir_ctx
{
  struct vnode *dir;
  off_t offset;
  int done;
  int err;
};

/*! Function callback type for block iterate function */
typedef int (*ext2_block_iter_t) (struct ext2_fs *, block_t *, blkcnt_t,
				  block_t, int, void *);

/*! Function callback type for directory iterate function */
typedef int (*ext2_dir_iter_t) (struct vnode *, int, struct ext2_dirent *, int,
				blksize_t, char *, void *);

static inline blkcnt_t
ext2_blocks_count (const struct ext2_super *s)
{
  return (blkcnt_t) s->s_blocks_count |
    (s->s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (blkcnt_t) s->s_blocks_count_hi << 32 : 0);
}

static inline void
ext2_blocks_count_set (struct ext2_super *s, blkcnt_t blocks)
{
  s->s_blocks_count = blocks;
  if (s->s_feature_incompat & EXT4_FT_INCOMPAT_64BIT)
    s->s_blocks_count_hi = blocks >> 32;
}

static inline void
ext2_blocks_count_add (struct ext2_super *s, blkcnt_t blocks)
{
  blkcnt_t temp = ext2_blocks_count (s) + blocks;
  ext2_blocks_count_set (s, temp);
}

static inline blkcnt_t
ext2_r_blocks_count (const struct ext2_super *s)
{
  return (blkcnt_t) s->s_r_blocks_count |
    (s->s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (blkcnt_t) s->s_r_blocks_count_hi << 32 : 0);
}

static inline void
ext2_r_blocks_count_set (struct ext2_super *s, blkcnt_t blocks)
{
  s->s_r_blocks_count = blocks;
  if (s->s_feature_incompat & EXT4_FT_INCOMPAT_64BIT)
    s->s_r_blocks_count_hi = blocks >> 32;
}

static inline void
ext2_r_blocks_count_add (struct ext2_super *s, blkcnt_t blocks)
{
  blkcnt_t temp = ext2_r_blocks_count (s) + blocks;
  ext2_r_blocks_count_set (s, temp);
}

static inline blkcnt_t
ext2_free_blocks_count (const struct ext2_super *s)
{
  return (blkcnt_t) s->s_free_blocks_count |
    (s->s_feature_incompat & EXT4_FT_INCOMPAT_64BIT ?
     (blkcnt_t) s->s_free_blocks_hi << 32 : 0);
}

static inline void
ext2_free_blocks_count_set (struct ext2_super *s, blkcnt_t blocks)
{
  s->s_free_blocks_count = blocks;
  if (s->s_feature_incompat & EXT4_FT_INCOMPAT_64BIT)
    s->s_free_blocks_hi = blocks >> 32;
}

static inline void
ext2_free_blocks_count_add (struct ext2_super *s, blkcnt_t blocks)
{
  blkcnt_t temp = ext2_free_blocks_count (s) + blocks;
  ext2_free_blocks_count_set (s, temp);
}

static inline int
ext2_has_group_desc_checksum (const struct ext2_super *s)
{
  return s->s_feature_ro_compat &
    (EXT4_FT_RO_COMPAT_GDT_CSUM | EXT4_FT_RO_COMPAT_METADATA_CSUM) ? 1 : 0;
}

static inline block_t
ext2_group_first_block (const struct ext2_fs *fs, unsigned int group)
{
  return fs->super.s_first_data_block +
    EXT2_GROUPS_TO_BLOCKS (fs->super, group);
}

static inline block_t
ext2_group_last_block (const struct ext2_fs *fs, unsigned int group)
{
  return group == fs->group_desc_count - 1 ?
    (block_t) ext2_blocks_count (&fs->super) - 1 :
    ext2_group_first_block (fs, group) + fs->super.s_blocks_per_group - 1;
}

static inline blkcnt_t
ext2_group_blocks_count (const struct ext2_fs *fs, unsigned int group)
{
  blkcnt_t nblocks;
  if (group == fs->group_desc_count - 1)
    {
      nblocks = (ext2_blocks_count (&fs->super) - fs->super.s_first_data_block)
	% fs->super.s_blocks_per_group;
      if (!nblocks)
	nblocks = fs->super.s_blocks_per_group;
    }
  else
    nblocks = fs->super.s_blocks_per_group;
  return nblocks;
}

static inline unsigned int
ext2_group_of_block (const struct ext2_fs *fs, block_t block)
{
  return (block - fs->super.s_first_data_block) / fs->super.s_blocks_per_group;
}

static inline unsigned int
ext2_group_of_inode (const struct ext2_fs *fs, ino_t inode)
{
  return (inode - 1) / fs->super.s_inodes_per_group;
}

static inline int
ext2_is_inline_symlink (const struct ext2_inode *inode)
{
  return S_ISLNK (inode->i_mode)
    && EXT2_I_SIZE (*inode) < sizeof (inode->i_block);
}

static inline int
ext2_needs_large_file (uint64_t size)
{
  return size >= 0x80000000UL;
}

static inline unsigned int
ext2_dir_rec_len (unsigned char name_len, int extended)
{
  unsigned int rec_len = name_len + EXT2_DIR_ENTRY_HEADER_LEN + EXT2_DIR_ROUND;
  rec_len &= ~EXT2_DIR_ROUND;
  if (extended)
    rec_len += EXT2_DIR_ENTRY_HASH_LEN;
  return rec_len;
}

__BEGIN_DECLS

extern const struct mount_ops ext2_mount_ops;
extern const struct vnode_ops ext2_vnode_ops;
extern struct ext2_bitmap_ops ext2_bitarray_ops;

/* Filesystem utility functions */

int ext2_bg_has_super (struct ext2_fs *fs, unsigned int group);
int ext2_bg_test_flags (struct ext2_fs *fs, unsigned int group, uint16_t flags);
void ext2_bg_clear_flags (struct ext2_fs *fs, unsigned int group,
			  uint16_t flags);
void ext2_update_super_revision (struct ext2_super *s);
int ext4_mmp_start (struct ext2_fs *fs);
int ext4_mmp_stop (struct ext2_fs *fs);
struct ext2_group_desc *ext2_group_desc (struct ext2_fs *fs,
					 struct ext2_group_desc *gdp,
					 unsigned int group);
struct ext4_group_desc *ext4_group_desc (struct ext2_fs *fs,
					 struct ext2_group_desc *gdp,
					 unsigned int group);
void ext2_super_bgd_loc (struct ext2_fs *fs, unsigned int group, block_t *super,
			 block_t *old_desc, block_t *new_desc, blkcnt_t *used);
block_t ext2_block_bitmap_loc (struct ext2_fs *fs, unsigned int group);
block_t ext2_inode_bitmap_loc (struct ext2_fs *fs, unsigned int group);
block_t ext2_inode_table_loc (struct ext2_fs *fs, unsigned int group);
block_t ext2_descriptor_block (struct ext2_fs *fs, block_t group_block,
			       unsigned int i);
uint32_t ext2_bg_free_blocks_count (struct ext2_fs *fs, unsigned int group);
void ext2_bg_free_blocks_count_set (struct ext2_fs *fs, unsigned int group,
				    uint32_t blocks);
uint32_t ext2_bg_free_inodes_count (struct ext2_fs *fs, unsigned int group);
void ext2_bg_free_inodes_count_set (struct ext2_fs *fs, unsigned int group,
				    uint32_t inodes);
uint32_t ext2_bg_used_dirs_count (struct ext2_fs *fs, unsigned int group);
void ext2_bg_used_dirs_count_set (struct ext2_fs *fs, unsigned int group,
				  uint32_t dirs);
uint32_t ext2_bg_itable_unused (struct ext2_fs *fs, unsigned int group);
void ext2_bg_itable_unused_set (struct ext2_fs *fs, unsigned int group,
				uint32_t unused);
int ext2_read_bitmap (struct ext2_fs *fs, int flags, unsigned int start,
		      unsigned int end);
int ext2_read_bitmaps (struct ext2_fs *fs);
int ext2_write_bitmaps (struct ext2_fs *fs);
void ext2_mark_bitmap (struct ext2_bitmap *bmap, uint64_t arg);
void ext2_unmark_bitmap (struct ext2_bitmap *bmap, uint64_t arg);
int ext2_test_bitmap (struct ext2_bitmap *bmap, uint64_t arg);
void ext2_mark_block_bitmap_range (struct ext2_bitmap *bmap, block_t block,
				   blkcnt_t num);
int ext2_set_bitmap_range (struct ext2_bitmap *bmap, uint64_t start,
			   unsigned int num, void *data);
int ext2_get_bitmap_range (struct ext2_bitmap *bmap, uint64_t start,
			   unsigned int num, void *data);
int ext2_find_first_zero_bitmap (struct ext2_bitmap *bmap, block_t start,
				 block_t end, block_t *result);
void ext2_cluster_alloc (struct ext2_fs *fs, ino_t ino,
			 struct ext2_inode *inode,
			 struct ext3_extent_handle *handle, block_t block,
			 block_t *physblock);
int ext2_map_cluster_block (struct ext2_fs *fs, ino_t ino,
			    struct ext2_inode *inode, block_t block,
			    block_t *physblock);
void ext2_inode_alloc_stats (struct ext2_fs *fs, ino_t ino, int inuse,
			     int isdir);
void ext2_block_alloc_stats (struct ext2_fs *fs, block_t block, int inuse);
int ext2_write_backup_superblock (struct ext2_fs *fs, unsigned int group,
				  block_t group_block, struct ext2_super *s);
int ext2_write_primary_superblock (struct ext2_fs *fs, struct ext2_super *s);
int ext2_flush_fs (struct ext2_fs *fs, int flags);
int ext2_open_file (struct ext2_fs *fs, ino_t inode, struct ext2_file *file);
int ext2_file_block_offset_too_big (struct ext2_fs *fs,
				    struct ext2_inode *inode, block_t offset);
int ext2_file_set_size (struct vnode *vp, off_t size);
int ext2_read_inode (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode);
int ext2_update_inode (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		       size_t bufsize);
void ext2_update_vfs_inode (struct vnode *vp);
int ext2_inode_set_size (struct ext2_fs *fs, struct ext2_inode *inode,
			 off_t size);
block_t ext2_find_inode_goal (struct ext2_fs *fs, ino_t ino,
			      struct ext2_inode *inode, block_t block);
int ext2_create_inode_cache (struct ext2_fs *fs, unsigned int cache_size);
void ext2_free_inode_cache (struct ext2_inode_cache *cache);
int ext2_flush_inode_cache (struct ext2_inode_cache *cache);
int ext2_file_flush (struct vnode *vp);
int ext2_sync_file_buffer_pos (struct vnode *vp);
int ext2_load_file_buffer (struct vnode *vp, int nofill);
int ext2_bmap (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
	       char *blockbuf, int flags, block_t block, int *retflags,
	       block_t *physblock);
int ext2_iblk_add_blocks (struct ext2_fs *fs, struct ext2_inode *inode,
			  block_t nblocks);
int ext2_iblk_sub_blocks (struct ext2_fs *fs, struct ext2_inode *inode,
			  block_t nblocks);
int ext2_iblk_set (struct ext2_fs *fs, struct ext2_inode *inode,
		   block_t nblocks);
int ext2_zero_blocks (struct ext2_fs *fs, block_t block, int num,
		      block_t *result, int *count);
int ext2_new_dir_block (struct ext2_fs *fs, ino_t ino, ino_t parent,
			char **block);
int ext2_write_dir_block (struct ext2_fs *fs, block_t block, char *buffer,
			  int flags, struct vnode *vp);
int ext2_new_block (struct ext2_fs *fs, block_t goal, struct ext2_bitmap *map,
		    block_t *result, struct ext2_balloc_ctx *ctx);
int ext2_new_inode (struct ext2_fs *fs, ino_t dir, struct ext2_bitmap *map,
		    ino_t *result);
int ext2_write_new_inode (struct ext2_fs *fs, ino_t ino,
			  struct ext2_inode *inode);
int ext2_new_file (struct vnode *dir, const char *name, mode_t mode,
		   struct vnode **result);
int ext2_alloc_block (struct ext2_fs *fs, block_t goal, char *blockbuf,
		      block_t *result, struct ext2_balloc_ctx *ctx);
int ext2_dealloc_blocks (struct ext2_fs *fs, ino_t ino,
			 struct ext2_inode *inode, char *blockbuf,
			 block_t start, block_t end);
int ext2_add_index_link (struct ext2_fs *fs, struct vnode *dir,
			 const char *name, ino_t ino, int flags);
int ext2_add_link (struct ext2_fs *fs, struct vnode *dir, const char *name,
		   ino_t ino, int flags);
int ext2_unlink_dirent (struct ext2_fs *fs, struct vnode *dir, const char *name,
			int flags);
int ext2_dir_type (mode_t mode);
int ext2_lookup_inode (struct ext2_fs *fs, struct vnode *dir, const char *name,
		       int namelen, char *buffer, ino_t *inode);
int ext2_expand_dir (struct vnode *dir);
int ext2_read_blocks (void *buffer, struct ext2_fs *fs, block_t block,
		      size_t num);
int ext2_write_blocks (const void *buffer, struct ext2_fs *fs, block_t block,
		       size_t num);
int ext2_get_rec_len (struct ext2_fs *fs, struct ext2_dirent *dirent,
		      unsigned int *rec_len);
int ext2_set_rec_len (struct ext2_fs *fs, unsigned int len,
		      struct ext2_dirent *dirent);
int ext2_block_iterate (struct ext2_fs *fs, struct vnode *dir, int flags,
			char *blockbuf, ext2_block_iter_t func, void *private);
int ext2_dir_iterate (struct ext2_fs *fs, struct vnode *dir, int flags,
		      char *blockbuf, ext2_dir_iter_t func, void *private);
void ext2_init_dirent_tail (struct ext2_fs *fs, struct ext2_dirent_tail *t);

/* Checksum functions */

uint32_t ext2_superblock_checksum (struct ext2_super *s);
int ext2_superblock_checksum_valid (struct ext2_fs *fs);
void ext2_superblock_checksum_update (struct ext2_fs *fs, struct ext2_super *s);
uint16_t ext2_bg_checksum (struct ext2_fs *fs, unsigned int group);
void ext2_bg_checksum_update (struct ext2_fs *fs, unsigned int group,
			      uint16_t checksum);
uint16_t ext2_group_desc_checksum (struct ext2_fs *fs, unsigned int group);
int ext2_group_desc_checksum_valid (struct ext2_fs *fs, unsigned int group);
void ext2_group_desc_checksum_update (struct ext2_fs *fs, unsigned int group);
uint32_t ext2_inode_checksum (struct ext2_fs *fs, ino_t ino,
			      struct ext2_large_inode *inode, int has_hi);
int ext2_inode_checksum_valid (struct ext2_fs *fs, ino_t ino,
			       struct ext2_large_inode *inode);
void ext2_inode_checksum_update (struct ext2_fs *fs, ino_t ino,
				 struct ext2_large_inode *inode);
int ext3_extent_block_checksum (struct ext2_fs *fs, ino_t ino,
				struct ext3_extent_header *eh, uint32_t *crc);
int ext3_extent_block_checksum_valid (struct ext2_fs *fs, ino_t ino,
				      struct ext3_extent_header *eh);
int ext3_extent_block_checksum_update (struct ext2_fs *fs, ino_t ino,
				       struct ext3_extent_header *eh);
uint32_t ext2_block_bitmap_checksum (struct ext2_fs *fs, unsigned int group);
int ext2_block_bitmap_checksum_valid (struct ext2_fs *fs, unsigned int group,
				      char *bitmap, int size);
void ext2_block_bitmap_checksum_update (struct ext2_fs *fs, unsigned int group,
					char *bitmap, int size);
uint32_t ext2_inode_bitmap_checksum (struct ext2_fs *fs, unsigned int group);
int ext2_inode_bitmap_checksum_valid (struct ext2_fs *fs, unsigned int group,
				      char *bitmap, int size);
void ext2_inode_bitmap_checksum_update (struct ext2_fs *fs, unsigned int group,
					char *bitmap, int size);
int ext2_dir_block_checksum_valid (struct ext2_fs *fs, struct vnode *dir,
				   struct ext2_dirent *dirent);
int ext2_dir_block_checksum_update (struct ext2_fs *fs, struct vnode *dir,
				    struct ext2_dirent *dirent);
uint32_t ext2_dirent_checksum (struct ext2_fs *fs, struct vnode *dir,
			       struct ext2_dirent *dirent, size_t size);
int ext2_dirent_checksum_valid (struct ext2_fs *fs, struct vnode *dir,
				struct ext2_dirent *dirent);
int ext2_dirent_checksum_update (struct ext2_fs *fs, struct vnode *dir,
				 struct ext2_dirent *dirent);
int ext2_dx_checksum (struct ext2_fs *fs, struct vnode *dir,
		      struct ext2_dirent *dirent, uint32_t *crc,
		      struct ext2_dx_tail **tail);
int ext2_dx_checksum_valid (struct ext2_fs *fs, struct vnode *dir,
			    struct ext2_dirent *dirent);
int ext2_dx_checksum_update (struct ext2_fs *fs, struct vnode *dir,
			     struct ext2_dirent *dirent);
int ext2_dir_block_checksum_valid (struct ext2_fs *fs, struct vnode *dir,
				   struct ext2_dirent *dirent);
int ext2_dir_block_checksum_update (struct ext2_fs *fs, struct vnode *dir,
				    struct ext2_dirent *dirent);

/* Extent functions */

int ext3_extent_open (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		      struct ext3_extent_handle **handle);
int ext3_extent_header_valid (struct ext3_extent_header *eh, size_t size);
int ext3_extent_goto (struct ext3_extent_handle *handle, int leaflvl,
		      block_t block);
int ext3_extent_get (struct ext3_extent_handle *handle, int flags,
		     struct ext3_generic_extent *extent);
int ext3_extent_get_info (struct ext3_extent_handle *handle,
			  struct ext3_extent_info *info);
int ext3_extent_node_split (struct ext3_extent_handle *handle, int canexpand);
int ext3_extent_fix_parents (struct ext3_extent_handle *handle);
int ext3_extent_insert (struct ext3_extent_handle *handle, int flags,
			struct ext3_generic_extent *extent);
int ext3_extent_replace (struct ext3_extent_handle *handle, int flags,
			 struct ext3_generic_extent *extent);
int ext3_extent_delete (struct ext3_extent_handle *handle, int flags);
int ext3_extent_dealloc_blocks (struct ext2_fs *fs, ino_t ino,
				struct ext2_inode *inode, block_t start,
				block_t end);
int ext3_extent_bmap (struct ext2_fs *fs, ino_t ino, struct ext2_inode *inode,
		      struct ext3_extent_handle *handle, char *blockbuf,
		      int flags, block_t block, int *retflags,
		      int *blocks_alloc, block_t *physblock);
int ext3_extent_set_bmap (struct ext3_extent_handle *handle, block_t logical,
			  block_t physical, int flags);
void ext3_extent_free (struct ext3_extent_handle *handle);

/* VFS layer functions */

int ext2_mount (struct mount *mp, unsigned int flags);
int ext2_unmount (struct mount *mp, unsigned int flags);
int ext2_check (struct vnode *vp);
void ext2_flush (struct mount *mp);

int ext2_lookup (struct vnode **result, struct vnode *dir, const char *name);
ssize_t ext2_read (struct vnode *vp, void *buffer, size_t len, off_t offset);
ssize_t ext2_write (struct vnode *vp, const void *buffer, size_t len,
		    off_t offset);
int ext2_sync (struct vnode *vp);
int ext2_create (struct vnode **result, struct vnode *dir, const char *name,
		 mode_t mode, dev_t rdev);
int ext2_mkdir (struct vnode **result, struct vnode *dir, const char *name,
		mode_t mode);
int ext2_rename (struct vnode *olddir, const char *oldname,
		 struct vnode *newdir, const char *newname);
int ext2_link (struct vnode *dir, struct vnode *vp, const char *name);
int ext2_unlink (struct vnode *dir, const char *name);
int ext2_symlink (struct vnode *dir, const char *name, const char *target);
off_t ext2_readdir (struct vnode *dir, struct dirent *dirent, off_t offset);
ssize_t ext2_readlink (struct vnode *vp, char *buffer, size_t len);
int ext2_truncate (struct vnode *vp, off_t len);
int ext2_fill (struct vnode *vp);
void ext2_dealloc (struct vnode *vp);

__END_DECLS

#endif
