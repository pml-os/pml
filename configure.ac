AC_PREREQ([2.69])
AC_INIT([PML], [0.1], [xnscdev@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

AC_CANONICAL_HOST

save_LDFLAGS="$LDFLAGS"
LDFLAGS="$save_LDFLAGS -nostartfiles -nostdlib"
save_LIBS="$LIBS"
LIBS="$save_LIBS -lgcc"

AC_PROG_CC
AM_PROG_AS
AC_PROG_RANLIB

case "$host_cpu" in
    x86_64 )
	ARCH=x86_64
	;;
    * )
	AC_MSG_ERROR([host CPU $host_cpu not supported])
	;;
esac
ARCH_DIR="arch/$ARCH"
AC_SUBST([ARCH])
AC_SUBST([ARCH_DIR])

AC_ARG_WITH([bootdir],
AS_HELP_STRING([--with-bootdir=DIR], [directory to install boot files]),
[bootdir="$withval"], [bootdir=/boot])
case "$bootdir" in
    /* ) ;;
    * )
	AC_MSG_ERROR([boot directory must be an absolute path])
	;;
esac
AC_SUBST([bootdir])

PML_CC_VEC

LDFLAGS="$save_LDFLAGS"
LIBS="$save_LIBS"

AM_CONDITIONAL([ARCH_X86_64], [test "$ARCH" = x86_64])

AC_CONFIG_LINKS([
kernel/boot.S:$ARCH_DIR/boot.S
include/pml/memory.h:include/pml/$ARCH/memory.h
include/pml/panic.h:include/pml/$ARCH/panic.h
include/pml/thread.h:include/pml/$ARCH/thread.h
])

AC_CONFIG_FILES([
Makefile
arch/x86_64/Makefile
drivers/Makefile
drivers/acpica/Makefile
include/Makefile
include/acpi/Makefile
include/pml/Makefile
kernel/Makefile
util/Makefile
])
AC_OUTPUT
