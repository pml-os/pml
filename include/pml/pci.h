/* pci.h -- This file is part of PML.
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

#ifndef __PML_PCI_H
#define __PML_PCI_H

/*! @file */

#include <pml/cdefs.h>

#define PCI_PORT_ADDRESS        0xcf8
#define PCI_PORT_DATA           0xcfc

/* PCI header types */

#define PCI_TYPE_DEVICE         0x00
#define PCI_TYPE_BRIDGE         0x01
#define PCI_TYPE_CARDBUS        0x02
#define PCI_TYPE_MULTI          0x80

/* PCI configuration space offsets */

#define PCI_VENDOR_ID           0x00
#define PCI_DEVICE_ID           0x02
#define PCI_COMMAND             0x04
#define PCI_STATUS              0x06
#define PCI_REVISION_ID         0x08
#define PCI_PROG_IF             0x09
#define PCI_SUBCLASS            0x0a
#define PCI_CLASS               0x0b
#define PCI_CACHE_LINE_SIZE     0x0c
#define PCI_LATENCY_TIMER       0x0d
#define PCI_HEADER_TYPE         0x0e
#define PCI_BIST                0x0f
#define PCI_BAR0                0x10
#define PCI_BAR1                0x14
#define PCI_BAR2                0x18
#define PCI_BAR3                0x1c
#define PCI_BAR4                0x20
#define PCI_SECONDARY_BUS       0x19

/* PCI device types */

#define PCI_DEVICE_BRIDGE       0x0604
#define PCI_DEVICE_NONE         0xffff

/*! Set if PCI BAR is accessed through I/O */
#define PCI_BAR_IO              (1 << 0)

/*! Maximum number of PCI devices on a bus */
#define PCI_DEVICES_PER_BUS     32

/*! Maximum number of functions per PCI device */
#define PCI_FUNCS_PER_DEVICE    8

/*! Represents the location of a PCI configuration space. */
typedef unsigned int pci_config_t;

/*!
 * Returns a value suitable for accessing the PCI configuration space through
 * the PCI I/O functions given information about a PCI device.
 *
 * @param bus the PCI bus
 * @param device the device number on the bus
 * @param func the function number on the device
 * @return the location of the configuration space
 */

static inline pci_config_t
pci_config (unsigned char bus, unsigned char device, unsigned char func)
{
  return ((unsigned int) bus << 16)
    | ((unsigned int) (device & 0x1f) << 11)
    | ((unsigned int) (func & 7) << 8);
}

__BEGIN_DECLS

unsigned char pci_inb (pci_config_t config, unsigned char offset);
unsigned short pci_inw (pci_config_t config, unsigned char offset);
unsigned int pci_inl (pci_config_t config, unsigned char offset);
void pci_outb (pci_config_t config, unsigned char offset, unsigned char value);
void pci_outw (pci_config_t config, unsigned char offset, unsigned short value);
void pci_outl (pci_config_t config, unsigned char offset, unsigned int value);

uint16_t pci_device_type (pci_config_t config);
pci_config_t pci_check_config (uint16_t vendor_id, uint16_t device_id,
			       pci_config_t config);
pci_config_t pci_check_device (uint16_t vendor_id, uint16_t device_id,
			       unsigned char bus, unsigned char device);
pci_config_t pci_enumerate_bus (uint16_t vendor_id, uint16_t device_id,
				unsigned char bus);
pci_config_t pci_find_device (uint16_t vendor_id, uint16_t device_id);

__END_DECLS

#endif
