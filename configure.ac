#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([html_when/ensure.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 memchr mkdir munmap setenv strcasecmp strchr strdup strrchr strstr strtol])

AC_CONFIG_FILES([Makefile
                 git-tools/Makefile
                 html_when/Makefile
                 html_when/git-tools/Makefile
                 html_when/libxmlfixes/Makefile
                 html_when/libxmlfixes/coolmake/Makefile
                 html_when/libxmlfixes/git-tools/Makefile])
AC_CONFIG_SUBDIRS([html_when/libxml2
                   html_when/libxmlfixes/libxml2
                   libxml2])
AC_OUTPUT
