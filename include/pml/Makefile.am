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

pmlincludedir = $(includedir)/pml

pmlinclude_HEADERS =	\
	cdefs.h		\
	lock.h		\
	memory.h	\
	panic.h		\
	tty.h		\
	types.h		\
	$(arch_headers)
