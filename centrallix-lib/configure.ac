dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/mtlexer.c)
AC_CONFIG_HEADERS(include/cxlibconfig.h)
AC_PREFIX_DEFAULT(/usr/local)

dnl Checks for programs.
AC_PROG_CC
CFLAGS=`echo $CFLAGS | sed -e "s/-O2//"`
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PATH_PROG([STRIP], [strip], [])
AC_PATH_PROG([AR], [ar], [])
AC_PROG_INSTALL

dnl Checks for libraries.
dnl Replace `main' with a function in -lz:
AC_CHECK_LIB(z, main)
AC_CHECK_LIB(crypt, main)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h sys/time.h syslog.h unistd.h shadow.h zlib.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(select socket strdup strerror strtod strtol strtoul endservent)

FINAL_TARGETS="libCentrallix.a"

dnl Build shared libCentrallix
SHARED_LIBCENTRALLIX

AC_SUBST(FINAL_TARGETS)
    
AC_OUTPUT(Makefile, echo timestamp > stamp-h)
