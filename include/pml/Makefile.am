include $(top_srcdir)/compile.mk

if ARCH_X86_64
arch_headers =		\
	cmos.h		\
	cpuid.h		\
	ctlreg.h	\
	gdt.h		\
	msr.h		\
	multiboot.h
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
	errno.h		\
	cdefs.h		\
	hpet.h		\
	lock.h		\
	pci.h		\
	pit.h		\
	tty.h		\
	types.h		\
	vgatext.h
	$(arch_headers)

EXTRA_DIST = x86_64
