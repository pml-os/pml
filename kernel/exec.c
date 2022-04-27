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

#include <pml/alloc.h>
#include <pml/elf.h>
#include <pml/memory.h>
#include <pml/mman.h>
#include <pml/syscall.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static char *
copy_string (struct elf_exec *exec, char *str)
{
  size_t len;
  size_t base_len;
  uintptr_t ptr;
  char *temp;
  char *s = str;
  const char *cptr;

  /* Determine length of string */
  ptr = vm_phys_addr (exec->old_pml4t, s);
  cptr = (char *) PHYS_REL (ptr);
  len = strnlen (cptr, PAGE_SIZE - (ptr & (PAGE_SIZE - 1)));
  base_len = len;
  if (len == PAGE_SIZE - (ptr & (PAGE_SIZE - 1)))
    {
      s = ALIGN_UP (s, PAGE_SIZE);
      while (1)
	{
	  size_t slen;
	  ptr = vm_phys_addr (exec->old_pml4t, s);
	  cptr = (char *) PHYS_REL (ptr);
	  slen = strnlen (cptr, PAGE_SIZE);
	  len += slen;
	  if (slen < PAGE_SIZE)
	    break;
	  s += PAGE_SIZE;
	}
    }

  /* Allocate space for argument list */
  if (!exec->arg_data)
    {
      size_t bytes = ALIGN_UP (len, PAGE_SIZE);
      exec->arg_data = sys_mmap (NULL, bytes, PROT_READ | PROT_WRITE,
				 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (UNLIKELY (exec->arg_data == MAP_FAILED))
	{
	  exec->arg_data = NULL;
	  return NULL;
	}
      exec->arg_ptr = exec->arg_data;
      exec->arg_len = bytes;
    }
  if ((size_t) (exec->arg_ptr + len - exec->arg_data) >= exec->arg_len)
    {
      if (expand_mmap (exec->pml4t, exec->arg_data,
		       exec->arg_ptr + len + 1 - exec->arg_data))
	return NULL;
    }

  /* Copy string to allocated area */
  temp = exec->arg_ptr;
  s = str;
  ptr = vm_phys_addr (exec->old_pml4t, s);
  cptr = (char *) PHYS_REL (ptr);
  exec->arg_ptr = mempcpy (exec->arg_ptr, cptr, base_len);
  s = ALIGN_UP (s, PAGE_SIZE);
  if (len >= PAGE_SIZE)
    {
      for (; base_len <= len - PAGE_SIZE; base_len += PAGE_SIZE)
	{
	  ptr = vm_phys_addr (exec->old_pml4t, s);
	  cptr = (char *) PHYS_REL (ptr);
	  exec->arg_ptr = mempcpy (exec->arg_ptr, cptr, PAGE_SIZE);
	  s += PAGE_SIZE;
	}
    }
  ptr = vm_phys_addr (exec->old_pml4t, s);
  cptr = (char *) PHYS_REL (ptr);
  exec->arg_ptr = mempcpy (exec->arg_ptr, cptr, len - base_len);
  *exec->arg_ptr++ = '\0';
  return temp;
}

/*!
 * Maps a continuous region of virtual memory. This function is used to
 * map memory to load an ELF file's code or data.
 *
 * @param base starting virtual address to map
 * @param len number of bytes to map
 * @param prot mmap-style protection flags for the memory region
 * @param vp the file to read
 * @param filesz number of bytes to read
 * @param offset offset in the file to read
 * @return zero on success
 */

int
elf_mmap (void *base, size_t len, int prot, struct vnode *vp, size_t filesz,
	  off_t offset)
{
  void *ptr;
  void *cptr;
  int flags = PAGE_FLAG_USER;
  if (prot & PROT_WRITE)
    flags |= PAGE_FLAG_RW;

  for (ptr = ALIGN_DOWN (base, PAGE_SIZE);
       ptr < ALIGN_UP (base + len, PAGE_SIZE); ptr += PAGE_SIZE)
    {
      uintptr_t page = alloc_page ();
      if (UNLIKELY (!page))
	goto err0;
      memset ((void *) PHYS_REL (page), 0, PAGE_SIZE);
      if (vm_map_page (THIS_THREAD->args.pml4t, page, ptr, PAGE_FLAG_RW))
	{
	  free_page (page);
	  goto err0;
	}
    }

  if (filesz && vfs_read (vp, base, filesz, offset) != (ssize_t) filesz)
    goto err0;

  for (ptr = ALIGN_DOWN (base, PAGE_SIZE);
       ptr < ALIGN_UP (base + len, PAGE_SIZE); ptr += PAGE_SIZE)
    {
      if (vm_map_page (THIS_THREAD->args.pml4t, physical_addr ((void *) ptr),
		       ptr, flags))
	goto err0;
    }
  return 0;

 err0:
  for (cptr = ALIGN_DOWN (base, PAGE_SIZE); cptr < ptr; cptr += PAGE_SIZE)
    free_page (physical_addr (cptr));
  return -1;
}

/*!
 * Loads the program headers of an ELF file into memory.
 *
 * @param exec the execution context
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
	  if (elf_mmap ((void *) phdr.p_vaddr, phdr.p_memsz, flags,
			vp, phdr.p_filesz, phdr.p_offset))
	    return -1;

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
  THIS_PROCESS->brk.curr = THIS_PROCESS->brk.base;
  THIS_PROCESS->brk.max = DATA_SEGMENT_MAX;
  return 0;
}

int
sys_execve (const char *path, char *const *argv, char *const *envp)
{
  struct vnode *vp = vnode_namei (path, 1);
  struct elf_exec exec;
  char **args;
  char **env;
  char **arrbuf;
  size_t len = 0;
  int ret;
  int i;
  if (!vp)
    return -1;

  /* Create the new PML4T structure with only the kernel-space memory copied */
  exec.pml4t_phys = alloc_page ();
  if (UNLIKELY (!exec.pml4t_phys))
    {
      UNREF_OBJECT (vp);
      RETV_ERROR (ENOMEM, -1);
    }
  exec.pml4t = (uintptr_t *) PHYS_REL (exec.pml4t_phys);
  memset (exec.pml4t, 0, PAGE_STRUCT_SIZE / 2);
  memcpy (exec.pml4t + PAGE_STRUCT_ENTRIES / 2,
	  THIS_THREAD->args.pml4t + PAGE_STRUCT_ENTRIES / 2,
	  PAGE_STRUCT_SIZE / 2);

  /* Save the old PML4T and load the new one */
  __asm__ volatile ("mov %%cr3, %0" : "=r" (exec.old_pml4t_phys));
  exec.old_pml4t = (uintptr_t *) PHYS_REL (exec.old_pml4t_phys);
  THIS_THREAD->args.pml4t = exec.pml4t;
  __asm__ volatile ("mov %0, %%cr3" :: "r" (exec.pml4t_phys));

  /* Load the ELF file into memory */
  ret = elf_load_file (&exec, vp);
  UNREF_OBJECT (vp);
  if (ret)
    goto err0;

  /* Setup the stack with argv and envp */
  exec.arg_data = NULL;
  exec.arg_ptr = NULL;
  exec.arg_len = 0;
  /* TODO Check that argv/envp doesn't exceed ARG_MAX */
  if (argv)
    {
      for (i = 0; argv[i]; i++)
	len++;
    }
  if (envp)
    {
      for (i = 0; envp[i]; i++)
	len++;
    }
  len += 2;
  arrbuf = sys_mmap (NULL, len * sizeof (char *), PAGE_FLAG_USER | PAGE_FLAG_RW,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (UNLIKELY (arrbuf == MAP_FAILED))
    goto err0;
  args = arrbuf;
  if (argv)
    {
      for (i = 0; argv[i]; i++)
	*arrbuf++ = copy_string (&exec, argv[i]);
    }
  *arrbuf++ = NULL;
  env = arrbuf;
  if (envp)
    {
      for (i = 0; envp[i]; i++)
	*arrbuf++ = copy_string (&exec, envp[i]);
    }
  *arrbuf++ = NULL;

  vm_unmap_user_mem (exec.old_pml4t);
  sched_exec (exec.entry, args, env);
  __builtin_unreachable ();

 err0:
  /* Restore the old PML4T before returning */
  THIS_THREAD->args.pml4t = exec.old_pml4t;
  __asm__ volatile ("mov %0, %%cr3" :: "r" (exec.old_pml4t_phys));
  free_page (exec.pml4t_phys);
  return -1;
}
