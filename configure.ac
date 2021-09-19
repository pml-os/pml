AC_PREREQ([2.69])
AC_INIT([pml], [0.1], [xnscdev@gmail.com])
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

AC_MSG_CHECKING([$CC options to enable vectorization])
AC_ARG_ENABLE([vectorize],
AS_HELP_STRING([--enable-vectorize=TYPE],
[build kernel with vectorization (default=sse2)]),
[vec_type="$enableval"], [vec_type=sse2])
case "$vec_type" in
    no* )
	vec_flags="-mno-mmx -mno-sse -mno-sse2"
	;;
    mmx | sse | sse2 | sse3 | ssse3 | sse4 | sse4a | sse4.1 | sse4.2 | avx | \
	avx2 )
	vec_flags="-ftree-vectorize -m$vec_type"
	;;
    avx512 )
	vec_flags="-ftree-vectorize -mavx512f"
	;;
    * )
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([invalid vectorization type $enableval
Type must be one of: mmx, sse, sse2, sse3, ssse3, sse4, sse4a, sse4.1, sse4.2,
avx, avx2, avx512])
	;;
esac
AC_MSG_RESULT([$vec_flags])
AC_SUBST([vec_flags])

LDFLAGS="$save_LDFLAGS"
LIBS="$save_LIBS"

AM_CONDITIONAL([ARCH_X86_64], [test "$ARCH" = x86_64])

AC_CONFIG_LINKS([
kernel/boot.S:$ARCH_DIR/boot.S
include/pml/memory.h:include/pml/$ARCH/memory.h
include/pml/panic.h:include/pml/$ARCH/panic.h
])

AC_CONFIG_FILES([
Makefile
arch/x86_64/Makefile
drivers/Makefile
include/pml/Makefile
kernel/Makefile
util/Makefile
])
AC_OUTPUT
