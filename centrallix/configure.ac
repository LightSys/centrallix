dnl Process this file with autoconf to produce a configure script.
AC_INIT(centrallix.c)
AC_CONFIG_HEADERS(config.h)
AC_PREFIX_DEFAULT(/usr/local)

dnl Checks for programs.
AC_PROG_CC
CFLAGS=${CFLAGS/-O2}
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PATH_PROG([STRIP], [strip], [])
AC_PATH_PROG([SED], [sed], [])
AC_PROG_INSTALL

dnl Checks for libraries.
dnl Replace `main' with a function in -lcrypt:
AC_CHECK_LIB(crypt, main)
dnl Replace `main' with a function in -ldl:
AC_CHECK_LIB(dl, main)
dnl Replace `main' with a function in -lm:
AC_CHECK_LIB(m, main)
dnl Replace `main' with a function in -lz:
AC_CHECK_LIB(z, main)

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h unistd.h readline/readline.h readline/history.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(gethostname mkdir rmdir socket strdup strspn strstr strtod strtol)

dnl Readline support
CENTRALLIX_CHECK_READLINE

dnl Centrallix specific tests.
CENTRALLIX_CHECK_CENTRALLIX
AH_TEMPLATE([CENTRALLIX_CONFIG], [Full path of config file])
CENTRALLIX_CONF_FILE
CENTRALLIX_OS_DIR

dnl HTTP osdriver support.
AH_TEMPLATE([USE_HTTP], [Define for HTTP support])
CENTRALLIX_CHECK_HTTP_OS

dnl Sybase support.
AH_TEMPLATE([USE_SYBASE], [Define for Sybase support])
CENTRALLIX_CHECK_SYBASE

dnl XML osdriver support.
AH_TEMPLATE([USE_XML], [Define for XML support])
CENTRALLIX_CHECK_XML_OS

AC_SUBST(OBJDRIVERMODULES)
AC_OUTPUT(Makefile, echo timestamp > stamp-h)
