include $(top_srcdir)/compile.mk

boot_PROGRAMS = kernel

component_libs =					\
	$(top_builddir)/$(ARCH_DIR)/libarch.a		\
	$(top_builddir)/drivers/libdrivers.a		\
	$(top_builddir)/drivers/fs/libfs.a		\
	$(top_builddir)/drivers/fs/ext2/libext2.a	\
	$(top_builddir)/util/libutil.a

link_script = $(top_srcdir)/$(ARCH_DIR)/link.ld

kernel_SOURCES =	\
	boot.S		\
	cmdline.c	\
	device.c	\
	entry.c		\
	exec.c		\
	fd.c		\
	heap.c		\
	mman.c		\
	pid.c		\
	process.c
if GDB_SCRIPTS
nodist_kernel_SOURCES = gdb-scripts.c
endif

kernel_LDFLAGS = -T $(link_script) -nostdlib -z max-page-size=4096
kernel_LDADD = $(component_libs) $(component_libs) -lgcc
kernel_DEPENDENCIES = $(link_script) $(component_libs)

BUILT_SOURCES = gdb-scripts.c
CLEANFILES = gdb-scripts.c

gdb-scripts.c: gdb-scripts.lst gdb-scripts.c.in gengdbscripts.sh
	$(AM_V_GEN)XXD=$(XXD) $(srcdir)/gengdbscripts.sh $< \
	    $(srcdir)/gdb-scripts.c.in $(top_srcdir)
