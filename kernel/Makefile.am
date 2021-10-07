include $(top_srcdir)/compile.mk

boot_PROGRAMS = kernel

component_libs =					\
	$(top_builddir)/$(ARCH_DIR)/libarch.a		\
	$(top_builddir)/drivers/libdrivers.a		\
	$(top_builddir)/util/libutil.a

link_script = $(top_srcdir)/$(ARCH_DIR)/link.ld

kernel_SOURCES =	\
	boot.S		\
	device.c	\
	entry.c		\
	heap.c		\
	pid.c
kernel_LDFLAGS = -T $(link_script) -nostdlib -z max-page-size=4096
kernel_LDADD = $(component_libs) $(component_libs) -lgcc
kernel_DEPENDENCIES = $(link_script) $(component_libs)
