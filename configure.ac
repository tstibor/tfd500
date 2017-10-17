#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

# Get version information from git.
m4_define([GIT_VERSION_STRING], m4_esyscmd([git describe --tags | sed 's/\(.*\)-.*/\1/' | tr -d '\n']))

AC_INIT([vsquery], [GIT_VERSION_STRING], [thomas@stibor.net], [], [https://github.com/tstibor/vsquery])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/tfd500.c])
AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC

# Use the C language and compiler for the following checks.
AC_LANG([C])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h sys/time.h termios.h fcntl.h])

# Propage flags and dirs among final Makefiles.
AC_SUBST([AM_CFLAGS])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_FUNC_STRTOD
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_MODE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

AC_CHECK_FUNCS([memset strchr strerror gettimeofday strptime])

AC_CONFIG_FILES([Makefile
		 src/Makefile])

# Remove unneeded libraries.
LDFLAGS="$LDFLAGS -Wl,--as-needed"

# Halt on warnings.
CFLAGS="$CFLAGS -Wall -Werror"

AC_OUTPUT
