/* pci.c -- This file is part of PML.
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

#include <pml/io.h>
#include <pml/pci.h>

/*!
 * Reads a one-byte value from a PCI configuration space register.
 *
 * @param config the PCI configuration space to use
 * @param offset the register offset to read from
 * @return the value that was read
 */

unsigned char
pci_inb (struct pci_config config, unsigned char offset)
{
  outl (pci_address (config) | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
  return (inl (PCI_PORT_DATA) >> ((offset & 3) * 8)) & 0xff;
}

/*!
 * Reads a two-byte value from a PCI configuration space register.
 *
 * @param config the PCI configuration space to use
 * @param offset the register offset to read from
 * @return the value that was read
 */

unsigned short
pci_inw (struct pci_config config, unsigned char offset)
{
  outl (pci_address (config) | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
  return (inl (PCI_PORT_DATA) >> ((offset & 2) * 8)) & 0xffff;
}

/*!
 * Reads a four-byte value from a PCI configuration space register.
 *
 * @param config the PCI configuration space to use
 * @param offset the register offset to read from
 * @return the value that was read
 */

unsigned int
pci_inl (struct pci_config config, unsigned char offset)
{
  outl (pci_address (config) | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
  return inl (PCI_PORT_DATA);
}

/*!
 * Writes a one-byte value from a PCI configuration space register.
 *
 * @param config the PCI configuration space to use
 * @param offset the register offset to write to
 * @param value the value to write
 */

void
pci_outb (struct pci_config config, unsigned char offset, unsigned char value)
{
  unsigned int x;
  outl (pci_address (config) | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
  x = inl (PCI_PORT_DATA) & ~(0xff << ((offset & 3) * 8));
  x |= (unsigned int) value << ((offset & 3) * 8);
  outl (x, PCI_PORT_DATA);
}

/*!
 * Writes a two-byte value from a PCI configuration space register.
 *
 * @param config the PCI configuration space to use
 * @param offset the register offset to write to
 * @param value the value to write
 */

void
pci_outw (struct pci_config config, unsigned char offset, unsigned short value)
{
  unsigned int x;
  outl (pci_address (config) | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
  x = inl (PCI_PORT_DATA) & ~(0xffff << ((offset & 2) * 8));
  x |= (unsigned int) value << ((offset & 2) * 8);
  outl (x, PCI_PORT_DATA);
}

/*!
 * Writes a four-byte value from a PCI configuration space register.
 *
 * @param config the PCI configuration space to use
 * @param offset the register offset to write to
 * @param value the value to write
 */

void
pci_outl (struct pci_config config, unsigned char offset, unsigned int value)
{
  outl (pci_address (config) | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
  outl (value, PCI_PORT_DATA);
}
