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
#include <pml/ata.h>
#include <pml/io.h>
#include <pml/memory.h>
#include <pml/pit.h>
#include <stdio.h>
#include <stdlib.h>

/*! Buffer used to temporarily store ATA identification data */
static unsigned char ata_buffer[2048];

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
	  uintptr_t prdt_addr;
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

	  /* Allocate a PRDT */
	  prdt_addr = alloc_page ();
	  ata_devices[count].prdt = (struct ata_prdt *) PHYS_REL (prdt_addr);

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
