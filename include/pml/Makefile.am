pmlincludedir = $(includedir)/pml

if ARCH_X86_64
arch_headers =		\
	asm.h		\
	cpuid.h		\
	ctlreg.h	\
	gdt.h		\
	io.h		\
	msr.h		\
	multiboot.h	\
	vgatext.h
else
arch_headers =
endif

pmlinclude_HEADERS =	\
	alloc.h		\
	errno.h		\
	cdefs.h		\
	lock.h		\
	memory.h	\
	panic.h		\
	tty.h		\
	types.h		\
	$(arch_headers)
