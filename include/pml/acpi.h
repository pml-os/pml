/* acpi.h -- This file is part of PML.
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

#ifndef __PML_ACPI_H
#define __PML_ACPI_H

/*!
 * @file
 * @brief ACPI macros and structure definitions
 */

#include <pml/cdefs.h>

/*!
 * Format of the RSDP before ACPI 2.0. This structure contains the signature,
 * checksum, revision, and a 32-bit pointer to only the RSDT.
 */

struct acpi_rsdp_old
{
  char signature[8];            /*!< RSDP signature (<tt>"RSD PTR "</tt>) */
  unsigned char checksum;
  char oem_id[6];
  unsigned char revision;       /*!< Nonzero if ACPI 2.0 or newer */
  uint32_t rsdt_addr;           /*!< Physical address of the RSDT */
};

/*!
 * Format of the RSDP after ACPI 2.0. This structure extends the old RSDP
 * (@ref acpi_rsdp_old) and includes the structure length and a 64-bit
 * pointer to the XSDT.
 */

struct acpi_rsdp
{
  struct acpi_rsdp_old old;     /*!< Old RSDP structure */
  uint32_t len;                 /*!< Length of RSDP in bytes */
  uint64_t xsdt_addr;           /*!< Physical address of the XSDT */
  unsigned char ext_checksum;
  unsigned char reserved[3];
};

/*!
 * Format of an ACPI table header. All ACPI tables begin with this structure
 * and can be identified with its @ref acpi_table_header.signature field.
 */

struct acpi_table_header
{
  char signature[4];            /*!< Table signature */
  uint32_t len;                 /*!< Length of table in bytes */
  unsigned char revision;
  unsigned char checksum;
  char oem_id[6];
  char oem_table_id[6];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
};

/*!
 * Format of the RSDT. Contains a header followed by an array of 32-bit
 * physical addresses to other ACPI tables.
 */

struct acpi_rsdt
{
  struct acpi_table_header header;
  uint32_t tables[];
};

/*!
 * Format of the XSDT in ACPI 2.0 and newer. Contains a header followed by
 * an array of 64-bit physical addresses to other ACPI tables.
 */

struct acpi_xsdt
{
  struct acpi_table_header header;
  uint64_t tables[];
};

/*! Values for possible types for MADT entries. */

enum acpi_madt_entry_type
{
  ACPI_MADT_ENTRY_LOCAL_APIC = 0,   /*!< Processor local APIC */
  ACPI_MADT_ENTRY_IOAPIC,           /*!< I/O APIC */
  ACPI_MADT_ENTRY_INT_SOURCE_OVR,   /*!< I/O APIC interrupt source override */
  ACPI_MADT_ENTRY_NMI_SOURCE,       /*!< I/O APIC NMI source */
  ACPI_MADT_ENTRY_LOCAL_APIC_NMI,   /*!< Local APIC NMI */
  ACPI_MADT_ENTRY_LOCAL_APIC_ADDR_OVR, /*!< Local APIC address override */
  ACPI_MADT_ENTRY_IOSAPIC,
  ACPI_MADT_ENTRY_LOCAL_SAPIC,
  ACPI_MADT_ENTRY_PLATFORM_INT_SOURCES,
  ACPI_MADT_ENTRY_LOCAL_X2APIC,     /*!< Processor local x2APIC */
  ACPI_MADT_ENTRY_LOCAL_X2APIC_NMI,
  ACPI_MADT_GIC_CPU_INTERFACE,
  ACPI_MADT_GIC_DISTRIBUTOR,
  ACPI_MADT_GIC_MSI_FRAME,
  ACPI_MADT_GIC_REDISTRIBUTOR,
  ACPI_MADT_GIC_INT_TRANSLATE_SERVICE
};

struct acpi_madt_entry
{
  unsigned char type;   /*!< Type of entry (see @ref acpi_madt_entry_type) */
  unsigned char len;    /*!< Length of entry */
};

/*! Set if the processor is ready to use */
#define ACPI_MADT_LOCAL_APIC_ENABLED        (1 << 0)
/*! Set if the processor can be enabled by system hardware */
#define ACPI_MADT_LOCAL_APIC_ONLINE_CAP     (1 << 1)

/*!
 * Format of the processor local APIC MADT entry. This entry corresponds
 * to a type of @ref ACPI_MADT_ENTRY_LOCAL_APIC and contains the processor UID
 * and local APIC ID.
 */

struct acpi_madt_local_apic
{
  struct acpi_madt_entry entry;     /*!< MADT entry header */
  unsigned char proc_uid;           /*!< ACPI processor UID */
  unsigned char local_apic_id;      /*!< Processor local APIC ID */
  uint32_t flags;                   /*!< Local APIC flags */
};

/*!
 * Format of the I/O APIC MADT entry. This entry corresponds to a type of
 * @ref ACPI_MADT_ENTRY_IOAPIC and contains the I/O APIC ID and physical
 * address.
 */

struct acpi_madt_ioapic
{
  struct acpi_madt_entry entry;     /*!< MADT entry header */
  unsigned char ioapic_id;          /*!< I/O APIC ID */
  unsigned char reserved;
  uint32_t ioapic_addr;             /*!< 32-bit physical address of I/O APIC */
  uint32_t gsi_base;                /*!< Global system interrupt base number */
};

/*!
 * Format of an I/O APIC interrupt source override MADT entry. This entry
 * corresponds to a type of @ref ACPI_MADT_ENTRY_INT_SOURCE_OVR and maps
 * an IRQ source to a global system interrupt.
 */

struct acpi_madt_int_source_ovr
{
  struct acpi_madt_entry entry;     /*!< MADT entry header */
  unsigned char bus;
  unsigned char source;             /*!< IRQ source */
  uint32_t gsi;                     /*!< Global system interrupt to signal */
  uint16_t flags;                   /*!< MPS INTI flags */
};

/*!
 * Format of a local APIC address override MADT entry. This entry corresponds
 * to a type of @ref ACPI_MADT_ENTRY_LOCAL_APIC_ADDR_OVR and contains the
 * 64-bit physical address of a local APIC.
 */

struct acpi_madt_local_apic_addr_ovr
{
  struct acpi_madt_entry entry;     /*!< MADT entry header */
  uint16_t reserved;
  uint64_t local_apic_addr;         /*!< Physical address of local APIC */
};

/*!
 * Format of the MADT. Contains a header followed by several variabl-length
 * entries.
 */

struct acpi_madt
{
  struct acpi_table_header header;  /*!< ACPI table header */
  uint32_t local_apic_addr;         /*!< Physical address of local APIC */
  uint32_t flags;                   /*!< MADT flags */
  unsigned char entries[];          /*!< Pointer to start of MADT entries */
};

/*!
 * Possible values for an ACPI address space.
 */

enum acpi_addr_space
{
  ACPI_ADDR_SPACE_MEMORY = 0,       /*!< System memory */
  ACPI_ADDR_SPACE_IO,               /*!< System I/O ports */
  ACPI_ADDR_SPACE_PCI_CONFIG,       /*!< PCI configuration space */
  ACPI_ADDR_SPACE_EMBEDDED,         /*!< Embedded controller */
  ACPI_ADDR_SPACE_SMB,              /*!< System management bus */
  ACPI_ADDR_SPACE_CMOS,             /*!< System CMOS */
  ACPI_ADDR_SPACE_PCI_BAR,          /*!< PCI device BAR */
  ACPI_ADDR_SPACE_IPMI,             /*!< System IPMI */
  ACPI_ADDR_SPACE_GPIO,             /*!< General purpose I/O */
  ACPI_ADDR_SPACE_SERIAL,           /*!< Serial bus */
  ACPI_ADDR_SPACE_COMM              /*!< Platform communication channel */
};

/*!
 * Format of a 12-byte ACPI extended address.
 */

struct acpi_addr
{
  unsigned char addr_space;     /*!< Address space, see @ref acpi_addr_space */
  unsigned char bit_width;
  unsigned char bit_offset;
  unsigned char access_size;    /*!< Required memory access size */
  uint64_t addr;                /*!< 64-bit physical address */
} __packed;

/*!
 * Format of the FADT since ACPI 2.0.
 */

struct acpi_fadt
{
  struct acpi_table_header header;  /*!< ACPI table header */
  uint32_t firmware_control;
  uint32_t dsdt;                    /*!< Physical address of the DSDT */
  unsigned char reserved;
  unsigned char power_profile;      /*!< Power management profile to use */
  uint16_t sci_int;
  uint32_t smi_command;
  unsigned char acpi_enable;
  unsigned char acpi_disable;
  unsigned char s4bios_req;
  unsigned char pstate_control;
  uint32_t pm1a_event_block;
  uint32_t pm1b_event_block;
  uint32_t pm1a_control_block;
  uint32_t pm1b_control_block;
  uint32_t pm2_control_block;
  uint32_t pm_timer_block;
  uint32_t gpe0_block;
  uint32_t gpe1_block;
  unsigned char pm1_event_len;
  unsigned char pm1_control_len;
  unsigned char pm2_control_len;
  unsigned char pm_timer_len;
  unsigned char gpe0_len;
  unsigned char gpe1_len;
  unsigned char gpe1_base;
  unsigned char cstate_control;
  uint16_t c2_latency;
  uint16_t c3_latency;
  uint16_t flush_size;
  uint16_t flush_stride;
  unsigned char duty_offset;
  unsigned char duty_width;
  unsigned char day_alarm;
  unsigned char month_alarm;
  unsigned char century;
  unsigned char iapc_boot_flags[2];
  unsigned char reserved2;
  uint32_t flags;
  struct acpi_addr reset_reg;
  unsigned char reset_value;
  unsigned char reserved3[3];
  uint64_t x_firmware_control;
  uint64_t x_dsdt;                  /*!< 64-bit physical address of the DSDT */
  struct acpi_addr x_pm1a_event_block;
  struct acpi_addr x_pm1b_event_block;
  struct acpi_addr x_pm1a_control_block;
  struct acpi_addr x_pm1b_control_block;
  struct acpi_addr x_pm2_control_block;
  struct acpi_addr x_pm_timer_block;
  struct acpi_addr x_gpe0_block;
  struct acpi_addr x_gpe1_block;
  struct acpi_addr sleep_control_reg;
  struct acpi_addr sleep_status_reg;
  uint64_t hyperv_id;
};

/*!
 * Format of the ACPI 2.0 table for the HPET.
 */

struct acpi_hpet
{
  struct acpi_table_header header;  /*!< ACPI table header */
  unsigned char revision;           /*!< Hardware revision ID */
  unsigned char count;              /*!< Comparator count and counter size */
  uint16_t vendor;                  /*!< PCI vendor ID of timer block */
  struct acpi_addr addr;            /*!< Address of event timer block */
  unsigned char number;             /*!< HPET sequence number */
  uint16_t clock_ticks;             /*!< Minimum clock ticks */
  unsigned char page_prot;          /*!< Page protection */
} __packed;

__BEGIN_DECLS

extern int acpi2;
extern struct acpi_rsdp *acpi_rsdp;
extern struct acpi_fadt *acpi_fadt;
extern struct acpi_table_header *acpi_dsdt;

void acpi_init (void);
void acpi_parse_table (const struct acpi_table_header *header);
void acpi_parse_rsdt (const struct acpi_rsdt *rsdt);
void acpi_parse_xsdt (const struct acpi_xsdt *xsdt);
void acpi_parse_fadt (const struct acpi_fadt *fadt);
void acpi_parse_madt (const struct acpi_madt *madt);
void acpi_parse_hpet (const struct acpi_hpet *hpet);
int acpi_rsdp_checksum (const struct acpi_rsdp *rsdp);
int acpi_table_checksum (const struct acpi_table_header *header);

__END_DECLS

#endif
