/* osl.c -- This file is part of PML.
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

#include <acpi/acpi.h>
#include <pml/lock.h>
#include <pml/memory.h>
#include <pml/panic.h>

/* ACPI OS-specific layer (OSL) for PML */

void *acpi_rsdp; /* Set to RSDP by Multiboot2 structure parser */

ACPI_STATUS
AcpiOsInitialize (void)
{
  return AE_OK;
}

ACPI_STATUS
AcpiOsTerminate (void)
{
  return AE_OK;
}

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (void)
{
  return (ACPI_PHYSICAL_ADDRESS) acpi_rsdp;
}

void *
AcpiOsMapMemory (ACPI_PHYSICAL_ADDRESS phys_addr, ACPI_SIZE len)
{
  return (void *) (phys_addr + LOW_PHYSICAL_BASE_VMA);
}

void
AcpiOsUnmapMemory (void *addr, ACPI_SIZE len)
{
}

ACPI_STATUS
AcpiOsGetPhysicalAddress (void *addr, ACPI_PHYSICAL_ADDRESS *phys_addr)
{
  uintptr_t ret = vm_phys_addr (kernel_pml4t, addr);
  if (!ret)
    return AE_BAD_ADDRESS;
  *phys_addr = ret;
  return AE_OK;
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock (ACPI_SPINLOCK l)
{
  spinlock_acquire (l);
  return 0;
}

void
AcpiOsReleaseLock (ACPI_SPINLOCK l, ACPI_CPU_FLAGS flags)
{
  spinlock_release (l);
}

void
acpi_init (void)
{
}
