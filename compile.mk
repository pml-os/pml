AM_CPPFLAGS = -DPML_KERNEL=1 -DARCH=$(ARCH) -I$(top_srcdir)/include \
	-I$(top_builddir)/include
AM_CFLAGS = -std=gnu99 -ffreestanding -mcmodel=large -mno-red-zone	\
	$(vec_flags) -Wall -Wextra -Wdeclaration-after-statement	\
	-Werror -Wno-unused-parameter

pmlincludedir = $(includedir)/pml
