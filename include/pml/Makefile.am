include $(top_srcdir)/compile.mk

arch_x86_64_headers =	\
	cmos.h		\
	cpuid.h		\
	ctlreg.h	\
	gdt.h		\
	msr.h		\
	multiboot.h

if ARCH_X86_64
arch_headers = $(arch_x86_64_headers)
else
arch_headers =
endif

nodist_pmlinclude_HEADERS = \
	io.h		\
	interrupt.h	\
	memory.h	\
	serial.h	\
	syscall.h	\
	thread.h

pmlinclude_HEADERS =	\
	acpi.h		\
	alloc.h		\
	asm.h		\
	ata.h		\
	cdefs.h		\
	devfs.h		\
	device.h	\
	dirent.h	\
	elf.h		\
	errno.h		\
	ext2fs.h	\
	fcntl.h		\
	hash.h		\
	hpet.h		\
	ioctl.h		\
	kbd.h		\
	lock.h		\
	map.h		\
	mman.h		\
	object.h	\
	panic.h		\
	pci.h		\
	pit.h		\
	process.h	\
	resource.h	\
	signal.h	\
	stat.h		\
	syslimits.h	\
	termios.h	\
	time.h		\
	tty.h		\
	types.h		\
	vfs.h		\
	vgatext.h	\
	wait.h
	$(arch_headers)

EXTRA_DIST = \
	x86_64 $(arch_x86_64_headers) \
	gensyscalls.py

syscall.h: $(top_srcdir)/$(ARCH_DIR)/syscalls.lst gensyscalls.py syscall.h.in
	$(AM_V_GEN)$(PYTHON) $(srcdir)/gensyscalls.py \
	    $(top_srcdir)/$(ARCH_DIR) $(srcdir)/syscall.h.in
