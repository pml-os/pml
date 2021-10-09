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
	panic.h		\
	thread.h

pmlinclude_HEADERS =	\
	acpi.h		\
	alloc.h		\
	asm.h		\
	ata.h		\
	devfs.h		\
	device.h	\
	errno.h		\
	cdefs.h		\
	hash.h		\
	hpet.h		\
	lock.h		\
	map.h		\
	object.h	\
	pci.h		\
	pit.h		\
	tty.h		\
	types.h		\
	vfs.h		\
	vgatext.h
	$(arch_headers)

EXTRA_DIST = \
	x86_64 $(arch_x86_64_headers)
