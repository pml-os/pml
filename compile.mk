AM_CPPFLAGS = -DPML_KERNEL=1 -I$(top_srcdir)/include
AM_CFLAGS = -std=gnu99 -ffreestanding -mcmodel=large -mno-red-zone	\
	$(vec_flags) -Wall -Wextra -Wdeclaration-after-statement	\
	-Wno-unused -Werror
