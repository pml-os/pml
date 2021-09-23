dnl -*- Autoconf -*-

# pml.m4 -- This file is part of PML.
# Copyright (C) 2021 XNSC
#
# PML is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PML is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PML. If not, see <https://www.gnu.org/licenses/>.

# PML_CC_VEC
# ------------------------------------------------------------------------------
# Adds an option to enable vectorization.
AC_DEFUN([PML_CC_VEC],
[AC_MSG_CHECKING([$CC options to enable vectorization])
AC_ARG_ENABLE([vectorize],
AS_HELP_STRING([--enable-vectorize=TYPE],
[build kernel with vectorization (default=sse2)]),
[vec_type="$enableval"], [vec_type=sse2])
case "$vec_type" in
    no* )
	vec_flags="-mno-mmx -mno-sse -mno-sse2"
	;;
    mmx | sse | sse2 | sse3 | ssse3 | sse4 | sse4a | sse4.1 | sse4.2 )
	vec_flags="-ftree-vectorize -m$vec_type"
	;;
    avx | avx2 )
        vec_flags="-ftree-vectorize -m$vec_type"
	AC_DEFINE([AVX_SUPPORT], [1], [Host CPU has AVX support])
	;;
    avx512* )
	vec_flags="-ftree-vectorize -mavx512f"
	AC_DEFINE([AVX512F_SUPPORT], [1], [Host CPU has AVX-512F support])
	;;
    * )
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([invalid vectorization type $enableval
Type must be one of: mmx, sse, sse2, sse3, ssse3, sse4, sse4a, sse4.1, sse4.2,
avx, avx2, avx512])
	;;
esac
AC_MSG_RESULT([$vec_flags])
AC_SUBST([vec_flags])])
