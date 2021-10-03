/* apic.c -- This file is part of PML.
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

#include <pml/acpi.h>
#include <pml/interrupt.h>
#include <pml/memory.h>
#include <pml/panic.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t ioapic_irq_map[IOAPIC_IRQ_COUNT];
static apic_id_t bsp_id;

apic_id_t local_apics[MAX_CORES];   /*!< IDs of local APICs */
size_t local_apic_count;            /*!< Number of local APICs */
void *local_apic_addr;              /*!< Address of CPU local APIC */
apic_id_t ioapic_id;                /*!< ID of I/O APIC */
void *ioapic_addr;                  /*!< Address of I/O APIC */
unsigned int ioapic_gsi_base;       /*!< I/O APIC GSI base */

/*!
 * Registers a local APIC from a MADT processor local APIC entry.
 *
 * @param entry the MADT entry
 */

static void
add_local_apic (const struct acpi_madt_local_apic *entry)
{
  if (local_apic_count < MAX_CORES
      && ((entry->flags & LOCAL_APIC_FLAG_ENABLED)
	  || (entry->flags & LOCAL_APIC_FLAG_ONLINE_CAP)))
    {
      local_apics[local_apic_count++] = entry->local_apic_id;
      printf ("ACPI: found local APIC (%#x)\n", entry->local_apic_id);
    }
}

/*!
 * Sets up information about the I/O APIC based on an I/O APIC MADT entry.
 *
 * @param entry the MADT entry
 */

static void
set_ioapic (const struct acpi_madt_ioapic *entry)
{
  ioapic_id = entry->ioapic_id;
  ioapic_addr = (void *) PHYS32_REL (entry->ioapic_addr);
  ioapic_gsi_base = entry->gsi_base;
  printf ("ACPI: found I/O APIC (%#x)\n", ioapic_id);
}

/*!
 * Overrides the address of the local APIC with the address specified
 * in a local APIC address override MADT entry.
 *
 * @param entry the MADT entry
 */

static void
set_local_apic_addr (const struct acpi_madt_local_apic_addr_ovr *entry)
{
  local_apic_addr = (void *) PHYS_REL (entry->local_apic_addr);
}

#ifdef USE_APIC

/*!
 * Starts the local APIC. This function is only called for the bootstrap
 * processor (BSP).
 */

void
int_start (void)
{
  size_t i;

  /* Make sure an I/O APIC is present */
  if (UNLIKELY (!ioapic_addr))
    panic ("No I/O APIC found");

  /* Start the local APIC */
  LOCAL_APIC_REG (LOCAL_APIC_REG_SPURIOUS_INT_VEC) = 0x1ff;

  /* Set I/O APIC IRQ mappings based on mapping table */
  for (i = 0; i < IOAPIC_IRQ_COUNT; i++)
    ioapic_set_irq (i, ioapic_irq_map[i]);
}

#endif

/*!
 * Clears any errors on the current CPU's local APIC.
 */

void
local_apic_clear_errors (void)
{
  LOCAL_APIC_REG (LOCAL_APIC_REG_ERR_STATUS) = 0;
}

/*!
 * Sends the end-of-interrupt signal to the local APIC to resume generating
 * interrupts.
 */

void
local_apic_eoi (void)
{
  LOCAL_APIC_REG (LOCAL_APIC_REG_EOI) = 0;
}

/*!
 * Sets up an IRQ override on the I/O APIC from an interrupt source override
 * MADT entry.
 *
 * @param entry the MADT entry
 */

void
ioapic_override_int (const struct acpi_madt_int_source_ovr *entry)
{
  unsigned char irq = entry->gsi - ioapic_gsi_base;
  int low_active = IOAPIC_FLAG_LOW_ACTIVE (entry->flags) == 3;
  int level_trigger = IOAPIC_FLAG_LEVEL_TRIGGER (entry->flags) == 3;
  ioapic_irq_map[irq] =
    ioapic_entry (0x20 + entry->source, bsp_id, IOAPIC_MODE_FIXED, low_active,
		  level_trigger);
}

/*!
 * Creates an I/O APIC entry.
 *
 * @param vector the interrupt vector number
 * @param apic_id the ID of the destination APIC
 * @param mode the interrupt delivery mode
 * @param low_active whether the interrupt is active when low
 * @param level_trigger whether the interrupt is level sensitive
 * @return an I/O APIC entry represented as a 64-bit integer
 */

uint64_t
ioapic_entry (unsigned char vector, apic_id_t apic_id, ioapic_mode_t mode,
	      int low_active, int level_trigger)
{
  return vector
    | (mode << 8)
    | (!!low_active << 13)
    | (!!level_trigger << 15)
    | ((uint64_t) (apic_id & 0x0f) << 56);
}

/*!
 * Sets up an interrupt mapping on the I/O APIC.
 *
 * @param irq the IRQ number
 * @param entry the I/O APIC entry, see @ref ioapic_entry
 */

void
ioapic_set_irq (unsigned char irq, uint64_t entry)
{
  ioapic_write (IOAPIC_REG_REDIR_LOW (irq), entry & 0xffffffff);
  ioapic_write (IOAPIC_REG_REDIR_HIGH (irq), entry >> 32);
}

/*!
 * Parses the ACPI MADT table to locate the address of the local APIC and
 * I/O APIC, and any interrupt source overrides.
 *
 * @param madt the address of the MADT
 */

void
acpi_parse_madt (const struct acpi_madt *madt)
{
#ifdef USE_APIC
  const unsigned char *ptr = madt->entries;
  const struct acpi_madt_entry *entry = (const struct acpi_madt_entry *) ptr;
  size_t i;

  /* Determine the APIC ID of the BSP */
  __asm__ volatile ("mov $1, %%eax; cpuid; shr $24, %%ebx" : "=b" (bsp_id));

  /* Identity map I/O APIC IRQ mappings to legacy values by default */
  for (i = 0; i < IOAPIC_IRQ_COUNT; i++)
    ioapic_irq_map[i] =
      ioapic_entry (0x20 + i, bsp_id, IOAPIC_MODE_FIXED, 0, 0);
  ioapic_irq_map[2] = 0; /* ISA IRQ2 doesn't exist */

  /* Parse MADT entries */
  for (i = offsetof (struct acpi_madt, entries); i < madt->header.len;
       i += entry->len, ptr += entry->len,
	 entry = (const struct acpi_madt_entry *) ptr)
    {
      switch (entry->type)
	{
	case ACPI_MADT_ENTRY_LOCAL_APIC:
	  add_local_apic ((const struct acpi_madt_local_apic *) entry);
	  break;
	case ACPI_MADT_ENTRY_IOAPIC:
	  set_ioapic ((const struct acpi_madt_ioapic *) entry);
	  break;
	case ACPI_MADT_ENTRY_INT_SOURCE_OVR:
	  ioapic_override_int ((const struct acpi_madt_int_source_ovr *) entry);
	  break;
	case ACPI_MADT_ENTRY_LOCAL_APIC_ADDR_OVR:
	  set_local_apic_addr ((const struct acpi_madt_local_apic_addr_ovr *)
			       entry);
	  break;
	}
    }

  /* If no local APIC address was given, guess */
  if (!local_apic_addr)
    local_apic_addr = (void *) PHYS32_REL (LOCAL_APIC_DEFAULT_ADDR);
#endif
}
