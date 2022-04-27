/* elf.h -- This file is part of PML.
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

#ifndef __PML_ELF_H
#define __PML_ELF_H

/*!
 * @file
 * @brief @c ELF file parsing
 */

#include <pml/vfs.h>
#include <elf.h>

/*!
 * Contains information needed to execute an ELF file.
 */

struct elf_exec
{
  void *entry;
  uintptr_t *pml4t;
  uintptr_t pml4t_phys;
  uintptr_t *old_pml4t;
  uintptr_t old_pml4t_phys;
  char *arg_data;
  char *arg_ptr;
  size_t arg_len;
};

__BEGIN_DECLS

int elf_mmap (void *base, size_t len, int prot, struct vnode *vp, size_t filesz,
	      off_t offset);
int elf_load_phdrs (Elf64_Ehdr *ehdr, struct vnode *vp);
int elf_load_file (struct elf_exec *exec, struct vnode *vp);

__END_DECLS

#endif
