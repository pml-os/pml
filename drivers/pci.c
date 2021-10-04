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
pci_inb (pci_config_t config, unsigned char offset)
{
  outl (config | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
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
pci_inw (pci_config_t config, unsigned char offset)
{
  outl (config | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
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
pci_inl (pci_config_t config, unsigned char offset)
{
  outl (config | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
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
pci_outb (pci_config_t config, unsigned char offset, unsigned char value)
{
  unsigned int x;
  outl (config | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
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
pci_outw (pci_config_t config, unsigned char offset, unsigned short value)
{
  unsigned int x;
  outl (config | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
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
pci_outl (pci_config_t config, unsigned char offset, unsigned int value)
{
  outl (config | (offset & 0xfc) | 0x80000000, PCI_PORT_ADDRESS);
  outl (value, PCI_PORT_DATA);
}

/*!
 * Determines the type of a PCI device.
 *
 * @param config the device's configuration space
 * @return a 16-bit value with the class in the top 8 bits and the subclass
 * in the bottom 8 bits
 */

uint16_t
pci_device_type (pci_config_t config)
{
  return (pci_inb (config, PCI_CLASS) << 8) | pci_inb (config, PCI_SUBCLASS);
}

/*!
 * Checks if a PCI device configuration with function set matches a vendor
 * and device ID.
 *
 * @param vendor_id the vendor ID to match
 * @param device_id the device ID to match
 * @param config the PCI configuration space
 * @return @ref config if the device matches, otherwise zero
 */

pci_config_t
pci_check_config (uint16_t vendor_id, uint16_t device_id, pci_config_t config)
{
  if (pci_device_type (config) == PCI_DEVICE_BRIDGE)
    config = pci_enumerate_bus (vendor_id, device_id,
				pci_inb (config, PCI_SECONDARY_BUS));
  return vendor_id == pci_inw (config, PCI_VENDOR_ID)
    && device_id == pci_inw (config, PCI_DEVICE_ID) ? config : 0;
}

/*!
 * Checks if a PCI device configuration matches a vendor and device ID.
 *
 * @param vendor_id the vendor ID to match
 * @param device_id the device ID to match
 * @param bus the PCI bus
 * @param device the PCI device number on the bus
 * @return the location of the PCI configuration space, or zero if no device
 * was found
 */

pci_config_t
pci_check_device (uint16_t vendor_id, uint16_t device_id, unsigned char bus,
		  unsigned char device)
{
  pci_config_t config = pci_config (bus, device, 0);
  pci_config_t dev;
  if (pci_inw (config, PCI_VENDOR_ID) == PCI_DEVICE_NONE)
    return 0; /* Device does not exist */
  dev = pci_check_config (vendor_id, device_id, config);
  if (dev)
    return dev;
  /* Check multiple device functions if supported */
  if (pci_inb (config, PCI_HEADER_TYPE) & PCI_TYPE_MULTI)
    {
      unsigned char func;
      for (func = 0; func < PCI_FUNCS_PER_DEVICE; func++)
	{
	  config = pci_config (bus, device, func);
	  if (pci_inw (config, PCI_VENDOR_ID) != PCI_DEVICE_NONE)
	    {
	      dev = pci_check_config (vendor_id, device_id, config);
	      if (dev)
		return dev;
	    }
	}
    }
  return 0;
}

/*!
 * Searches a PCI bus for a device matching a vendor and device ID.
 *
 * @param vendor_id the vendor ID of the target device
 * @param device_id the device ID of the target device
 * @param bus the bus number to search
 * @return the location of the device's PCI configuration space, or zero if
 * no device was found
 */

pci_config_t
pci_enumerate_bus (uint16_t vendor_id, uint16_t device_id, unsigned char bus)
{
  unsigned char device;
  for (device = 0; device < PCI_DEVICES_PER_BUS; device++)
    {
      pci_config_t config =
	pci_check_device (vendor_id, device_id, bus, device);
      if (config)
	return config;
    }
  return 0;
}

/*!
 * Finds a PCI device from a vendor and device ID.
 *
 * @param vendor_id the vendor ID of the target device
 * @param device_id the device ID of the target device
 * @return the location of the device's PCI configuration space, or zero if
 * no device was found
 */

pci_config_t
pci_find_device (uint16_t vendor_id, uint16_t device_id)
{
  pci_config_t config = pci_enumerate_bus (vendor_id, device_id, 0);
  unsigned char func;
  if (config)
    return config;
  for (func = 1; func < PCI_FUNCS_PER_DEVICE; func++)
    {
      config = pci_config (0, 0, func);
      if (pci_inw (config, PCI_VENDOR_ID) == PCI_DEVICE_NONE)
	break;
      config = pci_enumerate_bus (vendor_id, device_id, func);
      if (config)
	return config;
    }
  return 0;
}
