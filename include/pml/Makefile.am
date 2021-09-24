pmlincludedir = $(includedir)/pml

if ARCH_X86_64
arch_headers =		\
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

pmlinclude_HEADERS =	\
	alloc.h		\
	asm.h		\
	errno.h		\
	cdefs.h		\
	interrupt.h	\
	lock.h		\
	memory.h	\
	panic.h		\
	thread.h	\
	tty.h		\
	types.h		\
	$(arch_headers)
