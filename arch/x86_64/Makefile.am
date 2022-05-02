include $(top_srcdir)/compile.mk

noinst_LIBRARIES = libarch.a

BUILT_SOURCES =		\
	irq-stubs.S	\
	irq-vectors.c	\
	systab.c

nodist_libarch_a_SOURCES = \
	irq-stubs.S	\
	irq-vectors.c	\
	systab.c

libarch_a_SOURCES =	\
	apic.c		\
	exceptions.c	\
	exit.c		\
	fork.S		\
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
	pic8259.c	\
	rtc.c		\
	serial.c	\
	sched.S		\
	signal.S	\
	smp.c		\
	smp-init.S	\
	syscall.S	\
	syscall-init.c	\
	thread.c	\
	trampoline.S

# Generate interrupt handler stubs and IDT fill function

irq-vectors.c: interrupts.lst genirq.sh
	$(AM_V_GEN)$(srcdir)/genirq.sh $<

irq-stubs.S: irq-vectors.c

# Generate system call table

systab.c: syscalls.lst gensystab.py
	$(AM_V_GEN)$(PYTHON) $(srcdir)/gensystab.py $<

EXTRA_DIST =		\
	boot.S		\
	genirq.sh	\
	gensystab.py	\
	interrupts.lst	\
	syscalls.lst	\
	link.ld
