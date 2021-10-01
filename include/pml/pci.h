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
#define PCI_SECONDARY_BUS       0x19
#define PCI_BAR4                0x20

struct pci_config
{
  uint16_t bus;
  uint16_t device;
  uint16_t function;
};

static inline unsigned int
pci_address (struct pci_config config)
{
  return ((unsigned int) config.bus << 16)
    | ((unsigned int) (config.device & 0x1f) << 11)
    | ((unsigned int) (config.function & 7) << 8);
}

__BEGIN_DECLS

unsigned char pci_inb (struct pci_config config, unsigned char offset);
unsigned short pci_inw (struct pci_config config, unsigned char offset);
unsigned int pci_inl (struct pci_config config, unsigned char offset);
void pci_outb (struct pci_config config, unsigned char offset,
	       unsigned char value);
void pci_outw (struct pci_config config, unsigned char offset,
	       unsigned short value);
void pci_outl (struct pci_config config, unsigned char offset,
	       unsigned int value);

__END_DECLS

#endif
