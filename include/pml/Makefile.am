include $(top_srcdir)/compile.mk

if ARCH_X86_64
arch_headers =		\
	cmos.h		\
	cpuid.h		\
	ctlreg.h	\
	gdt.h		\
	io.h		\
	msr.h		\
	multiboot.h	\
	pit.h		\
	vgatext.h
else
arch_headers =
endif

nodist_pmlinclude_HEADERS = \
	interrupt.h	\
	memory.h	\
	panic.h		\
	thread.h

pmlinclude_HEADERS =	\
	alloc.h		\
	asm.h		\
	errno.h		\
	cdefs.h		\
	lock.h		\
	pci.h		\
	tty.h		\
	types.h		\
	$(arch_headers)

EXTRA_DIST = x86_64
