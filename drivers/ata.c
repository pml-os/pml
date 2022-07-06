/* ata.c -- This file is part of PML.
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

/*! @file */

#include <pml/alloc.h>
#include <pml/device.h>
#include <pml/io.h>
#include <pml/interrupt.h>
#include <pml/memory.h>
#include <pml/pit.h>
#include <pml/thread.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*! Buffer used to temporarily store ATA identification and I/O data */
static unsigned char ata_buffer[2048];

/*! Whether ATA DMA is supported */
static int ata_dma_support;

struct ata_registers ata_channels[2];
struct ata_device ata_devices[4];
volatile int ata_irq_recv;
pci_config_t ata_pci_config;

/*!
 * Reads a byte from an ATA register.
 *
 * @param channel the ATA channel to read from
 * @param reg the register to read
 * @return the value that was read, or zero if the register is invalid
 */

unsigned char
ata_read (enum ata_channel channel, unsigned char reg)
{
  unsigned char b = 0;
  if (reg > 0x07 && reg < 0x0c)
    ata_write (channel, ATA_REG_CONTROL, 0x80 | ata_channels[channel].nien);
  if (reg < 0x08)
    b = inb (ata_channels[channel].base + reg);
  else if (reg < 0x0c)
    b = inb (ata_channels[channel].base + reg - 0x06);
  else if (reg < 0x0e)
    b = inb (ata_channels[channel].control + reg - 0x0a);
  else if (reg < 0x16)
    b = inb (ata_channels[channel].bus_master_ide + reg - 0x0e);
  if (reg > 0x07 && reg < 0x0c)
    ata_write (channel, ATA_REG_CONTROL, ata_channels[channel].nien);
  return b;
}

/*!
 * Writes a byte to an ATA register.
 *
 * @param channel the ATA channel to read from
 * @param reg the register to read
 * @param value the value to write
 */

void
ata_write (enum ata_channel channel, unsigned char reg, unsigned char value)
{
  if (reg > 0x07 && reg < 0x0c)
    ata_write (channel, ATA_REG_CONTROL, 0x80 | ata_channels[channel].nien);
  if (reg < 0x08)
    outb (value, ata_channels[channel].base + reg);
  else if (reg < 0x0c)
    outb (value, ata_channels[channel].base + reg - 0x06);
  else if (reg < 0x0e)
    outb (value, ata_channels[channel].control + reg - 0x0a);
  else if (reg < 0x16)
    outb (value, ata_channels[channel].bus_master_ide + reg - 0x0e);
  if (reg > 0x07 && reg < 0x0c)
    ata_write (channel, ATA_REG_CONTROL, ata_channels[channel].nien);
}

/*!
 * Reads data from an ATA register into a buffer.
 *
 * @param channel the ATA channel to read from
 * @param reg the register to read
 * @param buffer the buffer to store the data
 * @param quads the number of quadwords to read
 */

void
ata_read_buffer (enum ata_channel channel, unsigned char reg, void *buffer,
		 size_t quads)
{
  if (reg > 0x07 && reg < 0x0c)
    ata_write (channel, ATA_REG_CONTROL, 0x80 | ata_channels[channel].nien);
  if (reg < 0x08)
    insl (ata_channels[channel].base + reg, buffer, quads);
  else if (reg < 0x0c)
    insl (ata_channels[channel].base + reg - 0x06, buffer, quads);
  else if (reg < 0x0e)
    insl (ata_channels[channel].control + reg - 0x0a, buffer, quads);
  else if (reg < 0x16)
    insl (ata_channels[channel].bus_master_ide + reg - 0x0e, buffer, quads);
  if (reg > 0x07 && reg < 0x0c)
    ata_write (channel, ATA_REG_CONTROL, ata_channels[channel].nien);
}

/*!
 * Polls an ATA drive and optionally checks for any errors.
 *
 * @param channel the channel of the ATA drive to poll
 * @param check_err whether to check for errors
 * @return <table>
 * <tr><th>Status bit</th><th>Value</th></tr>
 * <tr><td>No error</td><td>0</td></tr>
 * <tr><td>@ref ATA_SR_ERR</td><td>1</td></tr>
 * <tr><td>@ref ATA_SR_DF</td><td>2</td></tr>
 * <tr><td>@ref ATA_SR_DRQ</td><td>3</td></tr>
 * </table>
 */

int
ata_poll (unsigned char channel, unsigned char check_err)
{
  int i;
  for (i = 0; i < 4; i++)
    ata_read (channel, ATA_REG_ALT_STATUS);
  while (ata_read (channel, ATA_REG_STATUS) & ATA_SR_BSY)
    ;
  if (check_err)
    {
      unsigned char status = ata_read (channel, ATA_REG_STATUS);
      if (status & ATA_SR_ERR)
	return 1;
      if (status & ATA_SR_DF)
	return 2;
      if (!(status & ATA_SR_DRQ))
	return 3;
    }
  return 0;
}

/*!
 * Performs an I/O operation on an ATA drive.
 *
 * @param op the operation to perform
 * @param channel the ATA channel of the target drive
 * @param drive the ATA drive
 * @param lba the LBA of the first sector
 * @param sectors the number of sectors to read or write
 * @param buffer the buffer to read the sectors to or write from
 * @return zero on success
 */

int
ata_access (enum ata_op op, enum ata_channel channel, enum ata_drive drive,
	    unsigned int lba, unsigned char sectors, void *buffer)
{
  enum ata_addr_mode lba_mode;
  unsigned char lba_io[6];
  unsigned int device = channel * 2 + drive;
  unsigned int bus = ata_channels[channel].base;
  unsigned int words = ATA_SECTOR_SIZE / 2;
  unsigned short cylinder;
  unsigned char head;
  unsigned char sector;
  int err;
  char cmd;
  unsigned short i;
  unsigned char dma = ata_dma_support;

  /* Copy data to the ATA buffer if writing */
  if (op == ATA_OP_WRITE)
    memcpy (ata_devices[device].buffer, buffer, sectors * ATA_SECTOR_SIZE);

 retry:
  if (dma)
    {
      /* Setup PRDT for DMA */
      uintptr_t phys_addr;
      ata_devices[device].prdt.len = sectors * ATA_SECTOR_SIZE;
      phys_addr = (uintptr_t) &ata_devices[device].prdt - KERNEL_VMA;
      ata_write (channel, ATA_REG_BM_PRDT0, phys_addr & 0xff);
      ata_write (channel, ATA_REG_BM_PRDT1, (phys_addr >> 8) & 0xff);
      ata_write (channel, ATA_REG_BM_PRDT2, (phys_addr >> 16) & 0xff);
      ata_write (channel, ATA_REG_BM_PRDT3, (phys_addr >> 24) & 0xff);
    }
  else
    {
      /* Turn off interrupts */
      ata_irq_recv = 0;
      ata_channels[channel].nien = ATA_CTL_NIEN;
      ata_write (channel, ATA_REG_CONTROL, ata_channels[channel].nien);
    }

  /* Set parameters depending on addressing mode */
  if (lba >= 0x10000000)
    {
      /* Address is over 128G, require LBA48 */
      lba_mode = ATA_ADDR_LBA48;
      lba_io[0] = lba & 0xff;
      lba_io[1] = (lba >> 8) & 0xff;
      lba_io[2] = (lba >> 16) & 0xff;
      lba_io[3] = (lba >> 24) & 0xff;
      lba_io[4] = 0;
      lba_io[5] = 0;
      head = 0;
    }
  else if (ata_devices[device].capabilities & (1 << 9))
    {
      /* Use LBA28 if supported */
      lba_mode = ATA_ADDR_LBA28;
      lba_io[0] = lba & 0xff;
      lba_io[1] = (lba >> 8) & 0xff;
      lba_io[2] = (lba >> 16) & 0xff;
      lba_io[3] = 0;
      lba_io[4] = 0;
      lba_io[5] = 0;
      head = (lba >> 24) & 0xf;
    }
  else
    {
      /* Use CHS */
      lba_mode = ATA_ADDR_CHS;
      sector = lba % 63 + 1;
      cylinder = (lba - sector + 1) / 1008;
      lba_io[0] = sector;
      lba_io[1] = cylinder & 0xff;
      lba_io[2] = (cylinder >> 8) & 0xff;
      lba_io[3] = 0;
      lba_io[4] = 0;
      lba_io[5] = 0;
      head = (lba - sector + 1) % 1008 / 63;
    }

  /* Wait until the drive is not busy */
  while (ata_read (channel, ATA_REG_STATUS) & ATA_SR_BSY)
    ;

  /* Clear bus master error and interrupt if doing a DMA operation */
  if (dma)
    ata_write (channel, ATA_REG_BM_STATUS,
	       ata_read (channel, ATA_REG_BM_STATUS) &
	       ~(ATA_BM_SR_ERR | ATA_BM_SR_INT));

  /* Select drive */
  if (lba_mode == ATA_ADDR_CHS)
    ata_write (channel, ATA_REG_DEVICE_SELECT,
	       0xa0 | (ata_devices[device].drive << 4) | head);
  else
    ata_write (channel, ATA_REG_DEVICE_SELECT,
	       0xe0 | (ata_devices[device].drive << 4) | head);

  /* Write LBA and sector count */
  if (lba_mode == ATA_ADDR_LBA48)
    {
      ata_write (channel, ATA_REG_SECTOR_COUNT1, 0);
      ata_write (channel, ATA_REG_LBA3, lba_io[3]);
      ata_write (channel, ATA_REG_LBA4, lba_io[4]);
      ata_write (channel, ATA_REG_LBA5, lba_io[5]);
    }
  ata_write (channel, ATA_REG_SECTOR_COUNT0, sectors);
  ata_write (channel, ATA_REG_LBA0, lba_io[0]);
  ata_write (channel, ATA_REG_LBA1, lba_io[1]);
  ata_write (channel, ATA_REG_LBA2, lba_io[2]);

  /* Set the command */
  if (lba_mode == ATA_ADDR_LBA48)
    {
      if (dma)
	cmd = op == ATA_OP_WRITE ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT;
      else
	cmd = op == ATA_OP_WRITE ? ATA_CMD_WRITE_PIO_EXT : ATA_CMD_READ_PIO_EXT;
    }
  else
    {
      if (dma)
	cmd = op == ATA_OP_WRITE ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
      else
	cmd = op == ATA_OP_WRITE ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO;
    }
  ata_write (channel, ATA_REG_COMMAND, cmd);

  /* Run the command */
  if (dma)
    {
      int flags = ATA_BM_CMD_START;
      unsigned char status;
      if (op == ATA_OP_READ)
	flags |= ATA_BM_CMD_READ;

      /* Enable interrupts */
      ata_channels[channel].nien = 0;
      ata_write (channel, ATA_REG_CONTROL, ata_channels[channel].nien);

      /* Send the DMA command and wait for IRQ */
      while (!(ata_read (channel, ATA_REG_STATUS) & ATA_SR_DRQ))
	;
      ata_write (channel, ATA_REG_BM_COMMAND, flags);
      ata_await ();
      ata_write (channel, ATA_REG_BM_COMMAND, 0);

      status = ata_read (channel, ATA_REG_STATUS);
      if ((status & ATA_SR_ERR) || (status & ATA_SR_DF))
	return -1;

      /* Copy the data to the user buffer if reading */
      if (op == ATA_OP_READ)
	memcpy (buffer, ata_devices[device].buffer, sectors * ATA_SECTOR_SIZE);
    }
  else
    {
      /* Read/write data from I/O ports */
      if (op == ATA_OP_WRITE)
	{
	  for (i = 0; i < sectors; i++)
	    {
	      ata_poll (channel, 0);
	      outsw (bus, buffer, words);
	      buffer += words * 2;
	    }
	  ata_write (channel, ATA_REG_COMMAND,
		     lba_mode == ATA_ADDR_LBA48 ? ATA_CMD_CACHE_FLUSH_EXT :
		     ATA_CMD_CACHE_FLUSH);
	  ata_poll (channel, 0);
	}
      else
	{
	  for (i = 0; i < sectors; i++)
	    {
	      err = ata_poll (channel, 1);
	      if (err)
		return err;
	      insw (bus, buffer, words);
	      buffer += words * 2;
	    }
	}
    }
  return 0;
}

/*!
 * Waits until an ATA IRQ is issued, signaling the end of a DMA transfer.
 */

void
ata_await (void)
{
  while (!ata_irq_recv)
    ;
  ata_irq_recv = 0;
}

/*!
 * Reads sectors from an ATA drive.
 *
 * @param channel the channel of the ATA drive
 * @param drive the ATA drive
 * @param sectors number of sectors to read
 * @param lba LBA of first sector
 * @param buffer the buffer to store the data read
 * @return zero on sucess
 */

int
ata_read_sectors (enum ata_channel channel, enum ata_drive drive,
		  unsigned char sectors, unsigned int lba, void *buffer)
{
  unsigned int device = channel * 2 + drive;
  if (!ata_devices[device].exists || lba + sectors > ata_devices[device].size)
    RETV_ERROR (EINVAL, -1);
  return ata_access (ATA_OP_READ, channel, drive, lba, sectors, buffer);
}

/*!
 * Writes sectors to an ATA drive.
 *
 * @param channel the channel of the ATA drive
 * @param drive the ATA drive
 * @param sectors number of sectors to write
 * @param lba LBA of first sector
 * @param buffer the buffer containing the data to write
 * @return zero on sucess
 */

int
ata_write_sectors (enum ata_channel channel, enum ata_drive drive,
		   unsigned char sectors, unsigned int lba, const void *buffer)
{
  unsigned int device = channel * 2 + drive;
  if (!ata_devices[device].exists || lba + sectors > ata_devices[device].size)
    RETV_ERROR (EINVAL, -1);
  return ata_access (ATA_OP_WRITE, channel, drive, lba, sectors,
		     (void *) buffer);
}

/*!
 * Initializes the ATA driver. ATA devices connected to the system are polled
 * and prepared for I/O.
 */

void
ata_init (void)
{
  unsigned int bar0;
  unsigned int bar1;
  unsigned int bar2;
  unsigned int bar3;
  unsigned int bar4;
  unsigned short pci_cmd;
  unsigned char prog_if;
  int count = 0;
  int i;

  /* Read PCI configuration */
  ata_pci_config = pci_find_device (ATA_VENDOR_ID, ATA_DEVICE_ID);
  if (UNLIKELY (!ata_pci_config))
    {
      printf ("ATA: failed to find PCI device\n");
      return;
    }
  bar0 = pci_inl (ata_pci_config, PCI_BAR0);
  if (!bar0)
    bar0 = ATA_DEFAULT_BAR0;
  bar1 = pci_inl (ata_pci_config, PCI_BAR1);
  if (!bar1)
    bar1 = ATA_DEFAULT_BAR1;
  bar2 = pci_inl (ata_pci_config, PCI_BAR2);
  if (!bar2)
    bar2 = ATA_DEFAULT_BAR2;
  bar3 = pci_inl (ata_pci_config, PCI_BAR3);
  if (!bar3)
    bar3 = ATA_DEFAULT_BAR3;
  bar4 = pci_inl (ata_pci_config, PCI_BAR4);
  if (!bar4)
    {
      printf ("ATA: could not locate PCI BAR4\n");
      return;
    }

  /* Read the program interface byte from the PCI configuration space to
     determine if DMA is supported. DMA requires the controller to support
     bus mastering IDE and not be in native mode. */
  prog_if = pci_inb (ata_pci_config, PCI_PROG_IF);
  ata_dma_support = !!(prog_if & ATA_IF_BM_IDE);
  if (prog_if & ATA_IF_PRIMARY_NATIVE)
    {
      if (prog_if & ATA_IF_PRIMARY_TOGGLE)
	prog_if &= ~ATA_IF_PRIMARY_NATIVE;
      else
	ata_dma_support = 0;
    }
  if (prog_if & ATA_IF_SECONDARY_NATIVE)
    {
      if (prog_if & ATA_IF_SECONDARY_TOGGLE)
	prog_if &= ~ATA_IF_SECONDARY_NATIVE;
      else
	ata_dma_support = 0;
    }
  if (ata_dma_support)
    pci_outb (ata_pci_config, PCI_PROG_IF, prog_if);

  /* Enable PCI bus mastering */
  pci_cmd = pci_inw (ata_pci_config, PCI_COMMAND);
  if (!(pci_cmd & ATA_PCI_BUS_MASTER))
    pci_outw (ata_pci_config, PCI_COMMAND, pci_cmd | ATA_PCI_BUS_MASTER);

  /* Setup ATA channel registers */
  ata_channels[ATA_CHANNEL_PRIMARY].base = bar0 & ~3;
  ata_channels[ATA_CHANNEL_PRIMARY].control = bar1 & ~3;
  ata_channels[ATA_CHANNEL_SECONDARY].base = bar2 & ~3;
  ata_channels[ATA_CHANNEL_SECONDARY].control = bar3 & ~3;
  ata_channels[ATA_CHANNEL_PRIMARY].bus_master_ide = bar4 & ~3;
  ata_channels[ATA_CHANNEL_SECONDARY].bus_master_ide = (bar4 & ~3) + 8;

  /* Disable IRQs */
  ata_write (ATA_CHANNEL_PRIMARY, ATA_REG_CONTROL, ATA_CTL_NIEN);
  ata_write (ATA_CHANNEL_SECONDARY, ATA_REG_CONTROL, ATA_CTL_NIEN);

  /* Detect and assign IDE drives */
  printf ("ATA: Polling drives\n");
  for (i = 0; i < 2; i++)
    {
      int j;
      for (j = 0; j < 2; j++)
	{
	  unsigned char err = 0;
	  enum ata_mode type = ATA_MODE_ATA;
	  unsigned char status;
	  int k;

	  /* Select drive */
	  ata_write (i, ATA_REG_DEVICE_SELECT, 0xa0 | (j << 4));
	  pit_sleep (1);
	  ata_write (i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	  pit_sleep (1);

	  if (!ata_read (i, ATA_REG_STATUS))
	    continue;
	  while (1)
	    {
	      status = ata_read (i, ATA_REG_STATUS);
	      if (status & ATA_SR_ERR)
		{
		  err = 1;
		  break;
		}
	      if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
		break;
	    }

	  if (err)
	    {
	      /* Check if the drive is ATAPI */
	      unsigned char cl = ata_read (i, ATA_REG_LBA1);
	      unsigned char ch = ata_read (i, ATA_REG_LBA2);
	      if ((cl == 0x14 && ch == 0xeb) || (cl == 0x69 && ch == 0x96))
		type = ATA_MODE_ATAPI;
	      else
		continue;
	      ata_write (i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
	      pit_sleep (1);
	    }

	  /* Fill device properties */
	  ata_read_buffer (i, ATA_REG_DATA, ata_buffer, 128);
	  ata_devices[count].exists = 1;
	  ata_devices[count].type = type;
	  ata_devices[count].channel = i;
	  ata_devices[count].drive = j;
	  ata_devices[count].signature =
	    *((unsigned short *) (ata_buffer + ATA_IDENT_DEVICE_TYPE));
	  ata_devices[count].capabilities =
	    *((unsigned short *) (ata_buffer + ATA_IDENT_CAPABILITIES));
	  ata_devices[count].command_sets =
	    *((unsigned int *) (ata_buffer + ATA_IDENT_COMMAND_SETS));
	  if (ata_devices[count].command_sets & (1 << 26))
	    ata_devices[count].size =
	      *((unsigned int *) (ata_buffer + ATA_IDENT_MAX_LBA_EXT));
	  else
	    ata_devices[count].size =
	      *((unsigned int *) (ata_buffer + ATA_IDENT_MAX_LBA));
	  for (k = 0; k < 40; k += 2)
	    {
	      ata_devices[count].model[k] = ata_buffer[ATA_IDENT_MODEL + k + 1];
	      ata_devices[count].model[k + 1] = ata_buffer[ATA_IDENT_MODEL + k];
	    }
	  ata_devices[count].model[40] = '\0';

	  /* Setup PRDT */
	  ata_devices[count].prdt.addr =
	    (uintptr_t) ata_devices[count].buffer - KERNEL_VMA;
	  ata_devices[count].prdt.end = ATA_PRDT_END;

	  count++;
	}
    }

  /* Print detected drives */
  for (i = 0; i < 4; i++)
    {
      if (ata_devices[i].exists)
	printf ("ATA: %s %s: %s mode, %lu sectors\n",
		ata_devices[i].channel ? "secondary" : "primary",
		ata_devices[i].drive ? "slave" : "master",
		ata_devices[i].type ? "ATAPI" : "ATA", ata_devices[i].size);
    }
}

/*!
 * Reads data from a block device with an ATA drive backend.
 *
 * @param device the device to read from
 * @param buffer the buffer to store the read data
 * @param len number of bytes to read
 * @param offset in device file to start reading from
 * @param block whether to block while waiting for I/O
 * @return the number of bytes read, or -1 on failure
 */

ssize_t
ata_device_read (struct block_device *device, void *buffer, size_t len,
		 off_t offset, int block)
{
  struct disk_device_data *data;
  size_t start_diff;
  size_t end_diff;
  off_t start_lba;
  off_t mid_lba;
  off_t end_lba;
  off_t part_offset;
  size_t sectors;
  if (!len)
    return 0;

  start_lba = offset / ATA_SECTOR_SIZE;
  mid_lba = start_lba + !!(offset % ATA_SECTOR_SIZE);
  end_lba = (offset + len) / ATA_SECTOR_SIZE;
  sectors = end_lba - mid_lba;
  start_diff = mid_lba * ATA_SECTOR_SIZE - offset;
  end_diff = offset + len - end_lba * ATA_SECTOR_SIZE;

  /* Calculate LBA offset */
  data = device->device.data;
  part_offset = data->lba;
  if ((size_t) offset >= data->len)
    RETV_ERROR (EINVAL, -1);
  if (offset + len > data->len)
    len = data->len - offset;

  if (mid_lba > end_lba)
    {
      /* Completely contained in a single sector */
      if (ata_read_sectors (data->device->channel, data->device->drive, 1,
			    start_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
      memcpy (buffer, ata_buffer + ATA_SECTOR_SIZE - start_diff, len);
      return len;
    }

  /* Read full sectors */
  if (sectors)
    {
      /* The ATA driver can only read up to 255 sectors at a time, so read
	 in 255-sector chunks, then read the final remaining sectors
	 separately. */
      size_t i;
      for (i = 0; i + UCHAR_MAX < sectors; i += UCHAR_MAX)
	{
	  if (ata_read_sectors (data->device->channel, data->device->drive,
				UCHAR_MAX, mid_lba + part_offset + i,
				buffer + start_diff + i * ATA_SECTOR_SIZE))
	    RETV_ERROR (EIO, -1);
	}
      if (ata_read_sectors (data->device->channel, data->device->drive,
			    sectors - i, mid_lba + part_offset + i,
			    buffer + start_diff + i * ATA_SECTOR_SIZE))
	RETV_ERROR (EIO, -1);
    }

  /* Read unaligned starting bytes */
  if (start_diff)
    {
      if (ata_read_sectors (data->device->channel, data->device->drive, 1,
			    start_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
      memcpy (buffer, ata_buffer + ATA_SECTOR_SIZE - start_diff, start_diff);
    }

  /* Read unaligned ending bytes */
  if (end_diff)
    {
      if (ata_read_sectors (data->device->channel, data->device->drive, 1,
			    end_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
      memcpy (buffer + start_diff + sectors * ATA_SECTOR_SIZE, ata_buffer,
	      end_diff);
    }
  return len;
}

/*!
 * Writes data to a block device with an ATA drive backend.
 *
 * @param device the device to write to
 * @param buffer the buffer containing the data to write
 * @param len number of bytes to write
 * @param offset in device file to start writing to
 * @param block whether to block while waiting for I/O
 * @return the number of bytes written, or -1 on failure
 */

ssize_t
ata_device_write (struct block_device *device, const void *buffer, size_t len,
		  off_t offset, int block)
{
  struct disk_device_data *data;
  size_t start_diff;
  size_t end_diff;
  off_t start_lba;
  off_t mid_lba;
  off_t end_lba;
  off_t part_offset;
  size_t sectors;
  if (!len)
    return 0;

  start_lba = offset / ATA_SECTOR_SIZE;
  mid_lba = start_lba + !!(offset % ATA_SECTOR_SIZE);
  end_lba = (offset + len) / ATA_SECTOR_SIZE;
  sectors = end_lba - mid_lba;
  start_diff = mid_lba * ATA_SECTOR_SIZE - offset;
  end_diff = offset + len - end_lba * ATA_SECTOR_SIZE;

  /* Calculate LBA offset */
  data = device->device.data;
  part_offset = data->lba;
  if ((size_t) offset >= data->len)
    RETV_ERROR (EINVAL, -1);
  if (offset + len > data->len)
    len = data->len - offset;

  /* For writing data not on chunk boundaries, the original data is read
     into a temporary buffer and is partially overwritten by the new data,
     then the temporary buffer is written to disk. */
  if (mid_lba > end_lba)
    {
      /* Completely contained in a single sector */
      if (ata_read_sectors (data->device->channel, data->device->drive, 1,
			    start_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
      memcpy (ata_buffer + ATA_SECTOR_SIZE - start_diff, buffer, len);
      if (ata_write_sectors (data->device->channel, data->device->drive, 1,
			     start_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
      return len;
    }

  /* Write full sectors */
  if (sectors)
    {
      /* The ATA driver can only write up to 255 sectors at a time, so write
	 in 255-sector chunks, then write the final remaining sectors
	 separately. */
      size_t i;
      for (i = 0; i + UCHAR_MAX < sectors; i += UCHAR_MAX)
	{
	  if (ata_write_sectors (data->device->channel, data->device->drive,
				 UCHAR_MAX, mid_lba + part_offset + i,
				 buffer + start_diff + i * ATA_SECTOR_SIZE))
	    RETV_ERROR (EIO, -1);
	}
      if (ata_write_sectors (data->device->channel, data->device->drive,
			     sectors - i, mid_lba + part_offset + i,
			     buffer + start_diff + i * ATA_SECTOR_SIZE))
	RETV_ERROR (EIO, -1);
    }

  /* Write unaligned starting bytes */
  if (start_diff)
    {
      if (ata_read_sectors (data->device->channel, data->device->drive, 1,
			    start_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
      memcpy (ata_buffer + ATA_SECTOR_SIZE - start_diff, buffer, start_diff);
      if (ata_write_sectors (data->device->channel, data->device->drive, 1,
			     start_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
    }

  /* Write unaligned ending bytes */
  if (end_diff)
    {
      if (ata_read_sectors (data->device->channel, data->device->drive, 1,
			    end_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
      memcpy (ata_buffer, buffer + start_diff + sectors * ATA_SECTOR_SIZE,
	      end_diff);
      if (ata_write_sectors (data->device->channel, data->device->drive, 1,
			     end_lba + part_offset, ata_buffer))
	RETV_ERROR (EIO, -1);
    }
  return len;
}

void
int_ata_primary (void)
{
  if (ata_read (ATA_CHANNEL_PRIMARY, ATA_REG_BM_STATUS) & ATA_BM_SR_INT)
    {
      ata_irq_recv = 1;
      inb (ATA_REG_BM_STATUS);
    }
  EOI (14);
}

void
int_ata_secondary (void)
{
  if (ata_read (ATA_CHANNEL_SECONDARY, ATA_REG_BM_STATUS) & ATA_BM_SR_INT)
    {
      ata_irq_recv = 1;
      inb (ATA_REG_BM_STATUS);
    }
  EOI (15);
}
