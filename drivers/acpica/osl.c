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
#include <pml/io.h>
#include <pml/lock.h>
#include <pml/memory.h>
#include <pml/panic.h>
#include <pml/thread.h>
#include <stdio.h>
#include <stdlib.h>

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

ACPI_STATUS
AcpiOsPredefinedOverride (const ACPI_PREDEFINED_NAMES *obj, ACPI_STRING *new)
{
  *new = NULL;
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS
AcpiOsTableOverride (ACPI_TABLE_HEADER *table, ACPI_TABLE_HEADER **new)
{
  *new = NULL;
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS
AcpiOsPhysicalTableOverride (ACPI_TABLE_HEADER *table,
			     ACPI_PHYSICAL_ADDRESS *addr, UINT32 *len)
{
  *addr = 0;
  *len = 0;
  return AE_NOT_IMPLEMENTED;
}

void *
AcpiOsMapMemory (ACPI_PHYSICAL_ADDRESS phys_addr, ACPI_SIZE len)
{
  return (void *) PHYS_REL (phys_addr);
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

void *
AcpiOsAllocate (ACPI_SIZE size)
{
  return malloc (size);
}

void
AcpiOsFree (void *ptr)
{
  free (ptr);
}

ACPI_STATUS
AcpiOsCreateSemaphore (UINT32 max_units, UINT32 init_units,
		       ACPI_SEMAPHORE *handle)
{
  struct semaphore *sem = semaphore_create (init_units);
  if (UNLIKELY (!sem))
    return AE_NO_MEMORY;
  *handle = sem;
  return 0;
}

ACPI_STATUS
AcpiOsDeleteSemaphore (ACPI_SEMAPHORE handle)
{
  semaphore_free (handle);
  return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore (ACPI_SEMAPHORE handle, UINT32 units, UINT16 timeout)
{
  struct semaphore *sem = handle;
  UINT32 i;
  if (!timeout && sem->lock)
    return AE_TIME;
  for (i = 0; i < units; i++)
    semaphore_wait (sem);
  return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore (ACPI_SEMAPHORE handle, UINT32 units)
{
  UINT32 i;
  for (i = 0; i < units; i++)
    semaphore_signal (handle);
  return AE_OK;
}

ACPI_STATUS
AcpiOsCreateLock (ACPI_SPINLOCK *handle)
{
  lock_t *lock = malloc (sizeof (lock_t));
  if (UNLIKELY (!lock))
    return AE_NO_MEMORY;
  *lock = 0;
  *handle = (ACPI_SPINLOCK) lock;
  return AE_OK;
}

void
AcpiOsDeleteLock (ACPI_HANDLE handle)
{
  free (handle);
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
AcpiOsWaitEventsComplete (void)
{
}

ACPI_STATUS
AcpiOsReadPort (ACPI_IO_ADDRESS addr, UINT32 *value, UINT32 width)
{
  switch (width)
    {
    case 1:
      *value = inb (addr);
      break;
    case 2:
      *value = inw (addr);
      break;
    case 4:
    case 8:
      *value = inl (addr);
      break;
    default:
      return AE_BAD_PARAMETER;
    }
  return AE_OK;
}

ACPI_STATUS
AcpiOsWritePort (ACPI_IO_ADDRESS addr, UINT32 value, UINT32 width)
{
  switch (width)
    {
    case 1:
      outb (value, addr);
      break;
    case 2:
      outw (value, addr);
      break;
    case 4:
    case 8:
      outl (value, addr);
      break;
    default:
      return AE_BAD_PARAMETER;
    }
  return AE_OK;
}

ACPI_STATUS
AcpiOsReadMemory (ACPI_PHYSICAL_ADDRESS addr, UINT64 *value, UINT32 width)
{
  switch (width)
    {
    case 1:
      *value = *((unsigned char *) PHYS_REL (addr));
      break;
    case 2:
      *value = *((UINT16 *) PHYS_REL (addr));
      break;
    case 4:
      *value = *((UINT32 *) PHYS_REL (addr));
      break;
    case 8:
      *value = *((UINT64 *) PHYS_REL (addr));
      break;
    default:
      return AE_BAD_PARAMETER;
    }
  return AE_OK;
}

ACPI_STATUS
AcpiOsWriteMemory (ACPI_PHYSICAL_ADDRESS addr, UINT64 value, UINT32 width)
{
  switch (width)
    {
    case 1:
      *((unsigned char *) PHYS_REL (addr)) = value;
      break;
    case 2:
      *((UINT16 *) PHYS_REL (addr)) = value;
      break;
    case 4:
      *((UINT32 *) PHYS_REL (addr)) = value;
      break;
    case 8:
      *((UINT64 *) PHYS_REL (addr)) = value;
      break;
    default:
      return AE_BAD_PARAMETER;
    }
  return AE_OK;
}

ACPI_STATUS
AcpiOsSignal (UINT32 func, void *info)
{
  return AE_OK;
}

ACPI_THREAD_ID
AcpiOsGetThreadId (void)
{
  return THIS_THREAD->tid;
}

void
AcpiOsPrintf (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  vprintf (fmt, args);
  va_end (args);
}

void
AcpiOsVprintf (const char *fmt, va_list args)
{
  vprintf (fmt, args);
}

void
acpi_init (void)
{
  AcpiInitializeSubsystem ();
}
