include $(top_srcdir)/compile.mk

noinst_LIBRARIES = libarch.a

BUILT_SOURCES =		\
	irq-stubs.S	\
	irq-vectors.c	\
	systab.c	\
	syscall.h

nodist_libarch_a_SOURCES = \
	irq-stubs.S	\
	irq-vectors.c	\
	systab.c

libarch_a_SOURCES =	\
	apic.c		\
	clone.c		\
	exceptions.c	\
	gdt.c		\
	hwirq.c		\
	idt.c		\
	init.c		\
	insn-set.S	\
	lgdt.S		\
	memcpy.S	\
	memset.S	\
	mm.c		\
	multiboot.c	\
	page-fault.c	\
	panic.S		\
	panic-print.c	\
	pic8259.c	\
	pit.c		\
	rtc.c		\
	save-reg.S	\
	sched-tick.S	\
	smp.c		\
	smp-init.S	\
	syscall.S	\
	thread.c

# Generate interrupt handler stubs and IDT fill function

irq-vectors.c: interrupts.lst genirq.sh
	$(AM_V_GEN)$(srcdir)/genirq.sh $<

irq-stubs.S: irq-vectors.c

# Generate system call table and definitions

systab.c: syscalls.lst gensyscalls.sh
	$(AM_V_GEN)$(srcdir)/gensyscalls.sh $<

syscall.h: systab.c
	@test -f $@ || rm -f $<
	@test -f $@ || $(MAKE) $(AM_MAKEFLAGS) $<

nodist_pmlinclude_HEADERS = syscall.h

EXTRA_DIST =		\
	boot.S		\
	genirq.sh	\
	gensyscalls.sh	\
	interrupts.lst	\
	syscalls.lst	\
	link.ld
