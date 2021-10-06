/* ata.h -- This file is part of PML.
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

#ifndef __PML_ATA_H
#define __PML_ATA_H

/*!
 * @file
 * @brief Definitions for the ATA driver
 */

#include <pml/pci.h>

#define ATA_VENDOR_ID           0x8086    /*!< PCI vendor ID for PIIX3 IDE */
#define ATA_DEVICE_ID           0x7010    /*!< PCI device ID for PIIX3 IDE */

/* ATA PCI device programming interface bits */

#define ATA_IF_PRIMARY_NATIVE   (1 << 0)  /*!< Primary in PCI native mode */
#define ATA_IF_PRIMARY_TOGGLE   (1 << 1)  /*!< Primary mode toggleable */
#define ATA_IF_SECONDARY_NATIVE (1 << 2)  /*!< Secondary in PCI native mode */
#define ATA_IF_SECONDARY_TOGGLE (1 << 3)  /*!< Secondary mode toggleable */
#define ATA_IF_BM_IDE           (1 << 7)  /*!< Bus master IDE controller */

/* ATA status register bits */

#define ATA_SR_ERR              (1 << 0)  /*!< Error occurred */
#define ATA_SR_IDX              (1 << 1)  /*!< Index (always zero) */
#define ATA_SR_CORR             (1 << 2)  /*!< Corrected data (always zero) */
#define ATA_SR_DRQ              (1 << 3)  /*!< Data transfer requested */
#define ATA_SR_SRV              (1 << 4)  /*!< Overlapped service request */
#define ATA_SR_DF               (1 << 5)  /*!< Drive fault */
#define ATA_SR_RDY              (1 << 6)  /*!< Drive is ready */
#define ATA_SR_BSY              (1 << 7)  /*!< Preparing to send/receive data */

/* ATA error register bits */

#define ATA_ER_AMNF             (1 << 0)  /*!< Address mark not found */
#define ATA_ER_TK0NF            (1 << 1)  /*!< Track zero not found */
#define ATA_ER_ABRT             (1 << 2)  /*!< Aborted command */
#define ATA_ER_MCR              (1 << 3)  /*!< Media change request */
#define ATA_ER_IDNF             (1 << 4)  /*!< ID not found */
#define ATA_ER_MC               (1 << 5)  /*!< Media changed */
#define ATA_ER_UNC              (1 << 6)  /*!< Uncorrectable data error */
#define ATA_ER_BBK              (1 << 7)  /*!< Bad block detected */

/* ATA control register bits */

#define ATA_CTL_NIEN            (1 << 1)  /*!< Disable interrupts */

/* ATA bus master status register bits */

#define ATA_BM_SR_ERR           (1 << 1)  /*!< Error occurred */
#define ATA_BM_SR_INT           (1 << 2)  /*!< Interrupt bit */

/* ATA commands */

#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_READ_DMA        0xc8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_WRITE_DMA       0xca
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xe7
#define ATA_CMD_CACHE_FLUSH_EXT 0xea
#define ATA_CMD_PACKET          0xa0
#define ATA_CMD_IDENTIFY_PACKET 0xa1
#define ATA_CMD_IDENTIFY        0xec

/* ATA bus master commands */

#define ATA_BM_CMD_START        0x01
#define ATA_BM_CMD_READ         0x08

/* ATAPI commands */

#define ATAPI_CMD_READ          0xa8
#define ATAPI_CMD_EJECT         0x1b

/* ATA IDENTIFY command fields */

#define ATA_IDENT_DEVICE_TYPE   0
#define ATA_IDENT_CYLINDERS     2
#define ATA_IDENT_HEADS         6
#define ATA_IDENT_SECTORS       12
#define ATA_IDENT_SERIAL        20
#define ATA_IDENT_MODEL         54
#define ATA_IDENT_CAPABILITIES  98
#define ATA_IDENT_FIELD_VALID   106
#define ATA_IDENT_MAX_LBA       120
#define ATA_IDENT_COMMAND_SETS  164
#define ATA_IDENT_MAX_LBA_EXT   200

/* ATA convenience registers. The values of these macros do not necessarily
   correspond to the actual ATA register values! */

#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR           0x01
#define ATA_REG_FEATURES        0x01
#define ATA_REG_SECTOR_COUNT0   0x02
#define ATA_REG_LBA0            0x03
#define ATA_REG_LBA1            0x04
#define ATA_REG_LBA2            0x05
#define ATA_REG_DEVICE_SELECT   0x06
#define ATA_REG_COMMAND         0x07
#define ATA_REG_STATUS          0x07
#define ATA_REG_SECTOR_COUNT1   0x08
#define ATA_REG_LBA3            0x09
#define ATA_REG_LBA4            0x0a
#define ATA_REG_LBA5            0x0b
#define ATA_REG_CONTROL         0x0c
#define ATA_REG_ALT_STATUS      0x0c
#define ATA_REG_DEV_ADDR        0x0d
#define ATA_REG_BM_COMMAND      0x0e
#define ATA_REG_BM_STATUS       0x10
#define ATA_REG_BM_PRDT0        0x12
#define ATA_REG_BM_PRDT1        0x13
#define ATA_REG_BM_PRDT2        0x14
#define ATA_REG_BM_PRDT3        0x15

/* Default ATA PCI BAR registers */

#define ATA_DEFAULT_BAR0        0x1f0
#define ATA_DEFAULT_BAR1        0x3f6
#define ATA_DEFAULT_BAR2        0x170
#define ATA_DEFAULT_BAR3        0x376

/*! Set on ATA PCI BAR4 if PCI bus mastering is enabled */
#define ATA_PCI_BUS_MASTER      (1 << 2)

/*! Maximum number of physical region descriptor entries allowed */
#define ATA_PRDT_MAX            512

/*! Value in @ref ata_prdt.end to indicate the last PRDT entry. */
#define ATA_PRDT_END            0x8000

/*! Sector size for ATA drives. */
#define ATA_SECTOR_SIZE         512

/*! Sector size for ATAPI drives. */
#define ATAPI_SECTOR_SIZE       2048

/*! Possible values for an ATA channel. */

enum ata_channel
{
  ATA_CHANNEL_PRIMARY,          /*!< The primary channel */
  ATA_CHANNEL_SECONDARY         /*!< The secondary channel */
};

/*! Possible values for an ATA drive type. */

enum ata_drive
{
  ATA_DRIVE_MASTER,             /*!< The master drive of an ATA channel */
  ATA_DRIVE_SLAVE               /*!< The slave drive of an ATA channel */
};

/*! Possible values for an ATA interface mode. */

enum ata_mode
{
  ATA_MODE_ATA,                 /*!< ATA mode */
  ATA_MODE_ATAPI                /*!< ATAPI mode */
};

/*! Possible operations for I/O on an ATA drive. */

enum ata_op
{
  ATA_OP_READ,
  ATA_OP_WRITE
};

/*! Possible addressing modes for ATA PIO mode. */

enum ata_addr_mode
{
  ATA_ADDR_CHS,
  ATA_ADDR_LBA28,
  ATA_ADDR_LBA48
};

/*! Represents an entry in the ATA physical region descriptor table (PRDT). */

struct ata_prdt
{
  uint32_t addr;
  uint16_t len;
  uint16_t end;
};

/*! Convenience structure for storing ATA register bases. */

struct ata_registers
{
  uint16_t base;
  uint16_t control;
  uint16_t bus_master_ide;
  unsigned char nien;
};

/*!
 * Represents an ATA device and stores information detected by an ATA
 * @c IDENTIFY command.
 */

struct ata_device
{
  unsigned char exists;         /*!< Whether the device exists */
  enum ata_channel channel;     /*!< Channel type */
  enum ata_drive drive;         /*!< Drive type */
  enum ata_mode type;           /*!< ATA or ATAPI mode */
  unsigned short signature;     /*!< Drive signature */
  unsigned short capabilities;  /*!< Device capabilities */
  unsigned int command_sets;    /*!< Supported command sets */
  unsigned int size;            /*!< Size of the drive in sectors */
  unsigned char model[41];      /*!< Drive model string */
  struct ata_prdt prdt;         /*!< PRDT for DMA */
  unsigned char buffer[131072]; /*!< ATA device I/O buffer */
};

__BEGIN_DECLS

extern struct ata_registers ata_channels[2];
extern struct ata_device ata_devices[4];
extern volatile int ata_irq_recv;
extern pci_config_t ata_pci_config;

unsigned char ata_read (enum ata_channel channel, unsigned char reg);
void ata_write (enum ata_channel channel, unsigned char reg,
		unsigned char value);
void ata_read_buffer (enum ata_channel channel, unsigned char reg, void *buffer,
		      size_t quads);
int ata_poll (unsigned char channel, unsigned char check_err);
int ata_access (enum ata_op op, enum ata_channel channel, enum ata_drive drive,
		unsigned int lba, unsigned char sectors, void *buffer);
void ata_await (void);
int ata_read_sectors (enum ata_channel channel, enum ata_drive drive,
		      unsigned char sectors, unsigned int lba, void *buffer);
int ata_write_sectors (enum ata_channel channel, enum ata_drive drive,
		       unsigned char sectors, unsigned int lba,
		       const void *buffer);
void ata_init (void);

__END_DECLS

#endif
