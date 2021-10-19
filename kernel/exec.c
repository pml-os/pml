/* exec.c -- This file is part of PML.
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

#include <pml/elf.h>
#include <pml/process.h>
#include <pml/syscall.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*!
 * Loads the contents of an ELF file into memory.
 *
 * @param vp the vnode representing the ELF file to load
 * @return zero on success
 */

int
elf_load_file (struct vnode *vp)
{
  Elf64_Ehdr ehdr;
  if (vfs_read (vp, &ehdr, sizeof (Elf64_Ehdr), 0) != sizeof (Elf64_Ehdr)
      || ehdr.e_ident[EI_MAG0] != ELFMAG0
      || ehdr.e_ident[EI_MAG1] != ELFMAG1
      || ehdr.e_ident[EI_MAG2] != ELFMAG2
      || ehdr.e_ident[EI_MAG3] != ELFMAG3
      || ehdr.e_ident[EI_CLASS] != ELFCLASS64
      || ehdr.e_ident[EI_DATA] != ELFDATA2LSB
      || ehdr.e_ident[EI_VERSION] != EV_CURRENT
      || ehdr.e_type != ET_EXEC
      || ehdr.e_machine != ELF_MACHINE)
    RETV_ERROR (ENOEXEC, -1);
  debug_printf ("ELF header:\n"
		"Program header table: offset %u, entry size %u, %u entries\n"
		"Section header table: offset %u, entry size %u, %u entries\n"
		"String table index: %u\n",
		ehdr.e_phoff, ehdr.e_phentsize, ehdr.e_phnum,
		ehdr.e_shoff, ehdr.e_shentsize, ehdr.e_shnum,
		ehdr.e_shstrndx);
  return 0;
}

int
sys_execve (const char *path, char *const *argv, char *const *envp)
{
  struct vnode *vp = vnode_namei (path);
  int ret;
  if (!vp)
    return -1;
  ret = elf_load_file (vp);
  UNREF_OBJECT (vp);
  if (ret)
    return -1;
  RETV_ERROR (ENOSYS, -1);
}
