/* multiboot.h -- This file is part of PML.
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

/* Definitions for the Multiboot2 specification */

#ifndef __PML_MULTIBOOT_H
#define __PML_MULTIBOOT_H

#define MULTIBOOT_SEARCH                           32768
#define MULTIBOOT_HEADER_ALIGN                     8

#define MULTIBOOT2_HEADER_MAGIC                    0xe85250d6
#define MULTIBOOT2_BOOTLOADER_MAGIC                0x36d76289

#define MULTIBOOT_MOD_ALIGN                        0x00001000
#define MULTIBOOT_INFO_ALIGN                       0x00000008

#define MULTIBOOT_TAG_ALIGN                        8
#define MULTIBOOT_TAG_TYPE_END                     0
#define MULTIBOOT_TAG_TYPE_CMDLINE                 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME        2
#define MULTIBOOT_TAG_TYPE_MODULE                  3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO           4
#define MULTIBOOT_TAG_TYPE_BOOTDEV                 5
#define MULTIBOOT_TAG_TYPE_MMAP                    6
#define MULTIBOOT_TAG_TYPE_VBE                     7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER             8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS            9
#define MULTIBOOT_TAG_TYPE_APM                     10
#define MULTIBOOT_TAG_TYPE_EFI32                   11
#define MULTIBOOT_TAG_TYPE_EFI64                   12
#define MULTIBOOT_TAG_TYPE_SMBIOS                  13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD                14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW                15
#define MULTIBOOT_TAG_TYPE_NETWORK                 16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP                17
#define MULTIBOOT_TAG_TYPE_EFI_BS                  18
#define MULTIBOOT_TAG_TYPE_EFI32_IH                19
#define MULTIBOOT_TAG_TYPE_EFI64_IH                20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR          21

#define MULTIBOOT_HEADER_TAG_END                   0
#define MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST   1
#define MULTIBOOT_HEADER_TAG_ADDRESS               2
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS         3
#define MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS         4
#define MULTIBOOT_HEADER_TAG_FRAMEBUFFER           5
#define MULTIBOOT_HEADER_TAG_MODULE_ALIGN          6
#define MULTIBOOT_HEADER_TAG_EFI_BS                7
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI32   8
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI64   9
#define MULTIBOOT_HEADER_TAG_RELOCATABLE           10

#define MULTIBOOT_ARCHITECTURE_I386                0
#define MULTIBOOT_ARCHITECTURE_MIPS32              4
#define MULTIBOOT_HEADER_TAG_OPTIONAL              1

#define MULTIBOOT_LOAD_PREFERENCE_NONE             0
#define MULTIBOOT_LOAD_PREFERENCE_LOW              1
#define MULTIBOOT_LOAD_PREFERENCE_HIGH             2

#define MULTIBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED   1
#define MULTIBOOT_CONSOLE_FLAGS_EGA_TEXT_SUPPORTED 2

#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED         0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB             1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT        2

#ifndef __ASSEMBLER__

#include <pml/acpi.h>

struct mb_tag
{
  uint32_t type;
  uint32_t size;
};

struct mb_str_tag
{
  struct mb_tag tag;
  char string[];
};

struct mb_mod_tag
{
  struct mb_tag tag;
  uint32_t mod_start;
  uint32_t mod_end;
  char cmdline[];
};

struct mb_mmap_entry
{
  uint64_t base;
  uint64_t len;
  uint32_t type;
  uint32_t reserved;
};

struct mb_mmap_tag
{
  struct mb_tag tag;
  uint32_t entry_size;
  uint32_t entry_version;
  struct mb_mmap_entry entries[];
};

struct mb_acpi_rsdp_tag
{
  struct mb_tag tag;
  struct acpi_rsdp rsdp;
};

__BEGIN_DECLS

void multiboot_init (uintptr_t addr);

__END_DECLS

#endif /* !__ASSEMBLER__ */

#endif
