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
#include <pml/memory.h>
#include <pml/mman.h>
#include <pml/syscall.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*!
 * Loads the program headers of an ELF file into memory.
 *
 * @param ehdr the ELF file header
 * @param vp the vnode of the ELF file
 * @return zero on success
 */

int
elf_load_phdrs (Elf64_Ehdr *ehdr, struct vnode *vp)
{
  Elf64_Phdr phdr;
  size_t i;
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      if (vfs_read (vp, &phdr, sizeof (Elf64_Phdr), ehdr->e_phoff + i *
		    ehdr->e_phentsize) != sizeof (Elf64_Phdr))
	RETV_ERROR (EIO, -1);
      if (phdr.p_type == PT_LOAD)
	{
	  void *brk;
	  int flags = 0;
	  if (phdr.p_flags & PF_R)
	    flags |= PROT_READ;
	  if (phdr.p_flags & PF_W)
	    flags |= PROT_WRITE;
	  if (phdr.p_flags & PF_X)
	    flags |= PROT_EXEC;
	  if (phdr.p_vaddr + phdr.p_memsz > USER_MEMORY_LIMIT)
	    RETV_ERROR (EFAULT, -1);
	  if (add_mmap ((void *) phdr.p_vaddr, phdr.p_memsz, flags))
	    return -1;
	  if (phdr.p_filesz
	      && vfs_read (vp, (void *) phdr.p_vaddr, phdr.p_filesz,
			   phdr.p_offset) != (ssize_t) phdr.p_filesz)
	    RETV_ERROR (EIO, -1);

	  brk = (void *) ALIGN_UP (phdr.p_vaddr + phdr.p_memsz, PAGE_SIZE);
	  if (brk > THIS_PROCESS->brk.base)
	    THIS_PROCESS->brk.base = brk;
	}
    }
  return 0;
}

/*!
 * Loads the contents of an ELF file into memory.
 *
 * @param exec the ELF execution info structure to store info in
 * @param vp the vnode representing the ELF file to load
 * @return zero on success
 */

int
elf_load_file (struct elf_exec *exec, struct vnode *vp)
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
  THIS_PROCESS->brk.base = 0;
  if (elf_load_phdrs (&ehdr, vp))
    return -1;
  exec->entry = (void *) ehdr.e_entry;
  return 0;
}

int
sys_execve (const char *path, char *const *argv, char *const *envp)
{
  struct vnode *vp = vnode_namei (path, 1);
  struct elf_exec exec;
  int ret;
  if (!vp)
    return -1;
  ret = elf_load_file (&exec, vp);
  UNREF_OBJECT (vp);
  if (ret)
    return -1;
  sched_exec (exec.entry);
}
