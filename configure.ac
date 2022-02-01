AC_PREREQ([2.69])
AC_INIT([PML], [0.1], [xnscdev@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([pml-config.h])
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

AC_CANONICAL_HOST

save_CFLAGS="$CFLAGS"
CFLAGS="$save_CFLAGS -ffreestanding"
save_LDFLAGS="$LDFLAGS"
LDFLAGS="$save_LDFLAGS -nostartfiles -nostdlib"
save_LIBS="$LIBS"
LIBS="$save_LIBS -lgcc"

AC_PROG_CC
AM_PROG_AS
AC_PROG_RANLIB
AM_PATH_PYTHON([3.5])

case "$host_cpu" in
    x86_64 )
	ARCH=x86_64
	ELF_MACHINE=EM_X86_64
	;;
    * )
	AC_MSG_ERROR([host CPU $host_cpu not supported])
	;;
esac
ARCH_DIR="arch/$ARCH"
AC_SUBST([ARCH])
AC_SUBST([ARCH_DIR])
AC_DEFINE_UNQUOTED([ELF_MACHINE], [$ELF_MACHINE],
[Required value in the e_machine field of an ELF header])

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

AC_ARG_WITH([gdb-script],
AS_HELP_STRING([--with-gdb-script], [embed GDB script in kernel executable]),
[if test "x$with_gdb_script" != xno; then
    AC_PATH_PROG([XXD], [xxd], [no])
    if test "x$XXD" = xno; then
	AC_MSG_ERROR([xxd program is required to build inline GDB script])
    fi
    AC_SUBST([XXD])
fi], [with_gdb_script=no])
AM_CONDITIONAL([GDB_SCRIPT], [test "x$with_gdb_script" != xno])

PML_DEFAULT_FEATURE([apic], [USE_APIC],
[disable interrupt management with the APIC],
[Use the APIC for interrupt management])

PML_DEFAULT_FEATURE([smp], [ENABLE_SMP],
[disable support for multiple processors],
[Enable symmetric multiprocessing])

PML_CC_VEC

AC_CHECK_HEADERS([elf.h], [], AC_MSG_ERROR([elf.h not found]))

CFLAGS="$save_CFLAGS"
LDFLAGS="$save_LDFLAGS"
LIBS="$save_LIBS"

AM_CONDITIONAL([ARCH_X86_64], [test "$ARCH" = x86_64])

AC_CONFIG_LINKS([
kernel/boot.S:$ARCH_DIR/boot.S
include/pml/io.h:include/pml/$ARCH/io.h
include/pml/interrupt.h:include/pml/$ARCH/interrupt.h
include/pml/memory.h:include/pml/$ARCH/memory.h
include/pml/panic.h:include/pml/$ARCH/panic.h
include/pml/serial.h:include/pml/$ARCH/serial.h
include/pml/thread.h:include/pml/$ARCH/thread.h
])

AC_CONFIG_FILES([
Makefile
arch/x86_64/Makefile
drivers/Makefile
drivers/fs/Makefile
drivers/fs/ext2/Makefile
include/Makefile
include/pml/Makefile
kernel/Makefile
util/Makefile
])
AC_OUTPUT
