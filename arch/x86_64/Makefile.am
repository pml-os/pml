include $(top_srcdir)/compile.mk

noinst_LIBRARIES = libarch.a

BUILT_SOURCES =		\
	irq-stubs.S	\
	irq-vectors.c

libarch_a_SOURCES =	\
	exceptions.c	\
	gdt.c		\
	hwirq.c		\
	idt.c		\
	init.c		\
	insn-set.S	\
	irq-stubs.S	\
	irq-vectors.c	\
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
	save-reg.S	\
	sched-tick.S	\
	thread.c

# Generate interrupt handler stubs and IDT fill function

irq-vectors.c: interrupts.lst genirq.sh
	$(AM_V_GEN)$(srcdir)/genirq.sh $<

irq-stubs.S: irq-vectors.c
