#                                               -*- Autoconf -*-
# -----------------------------------------------------------------------------
# Process this file with autoconf to produce a configure script.
# -----------------------------------------------------------------------------

AC_PREREQ(2.50)
AC_INIT(pika, 0.1, russell at pika-lang.org)
AC_CONFIG_SRCDIR([libpika/Pika.h])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([libpika/config.h])

# -----------------------------------------------------------------------------
# Checks for tools.
# -----------------------------------------------------------------------------

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_RANLIB

# -----------------------------------------------------------------------------
# Checks for libraries.
# -----------------------------------------------------------------------------

AC_CHECK_LIB(dld, shl_load)
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(m, pow)

# -----------------------------------------------------------------------------
# Checks for header files.
# -----------------------------------------------------------------------------

AC_CHECK_HEADERS([errno.h malloc.h memory.h stdlib.h stdio.h ctype.h unistd.h])
AC_CHECK_HEADERS([dl.h sys/dl.h dld.h])
AC_CHECK_HEADERS([string.h strings.h])
AC_CHECK_HEADERS([stdint.h])

#AC_CHECK_HEADERS([sys/param.h])
#AC_CHECK_HEADERS([sys/types.h])

# -----------------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Checks for library functions.
# -----------------------------------------------------------------------------

AC_CHECK_FUNCS(strchr index)
AC_CHECK_FUNCS(strrchr rindex)
AC_CHECK_FUNCS(memcpy strcpy bcopy)
AC_CHECK_FUNCS(bzero)
AC_CHECK_FUNCS(memset)
AC_CHECK_FUNCS(strtok_r strtok_s strtok)
# -----------------------------------------------------------------------------
# Makefiles to create.
# -----------------------------------------------------------------------------

AC_OUTPUT(Makefile libpika/Makefile pikac/Makefile)
