dnl Process this file with autoconf to produce a configure script.
AC_INIT([Centrallix Application Server], 
	esyscmd(echo -n $(cat .version)),
	[centrallix-devel@lists.sourceforge.net])
AC_CONFIG_HEADERS(config.h)
AC_PREFIX_DEFAULT(/usr/local)

dnl Checks for programs.
AC_PROG_CC
CFLAGS=`echo $CFLAGS | sed -e "s/-O2//"`
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PATH_PROG([STRIP], [strip], [])
AC_PATH_PROG([SED], [sed], [])
AC_PROG_INSTALL
AC_CACHE_SAVE

CHECK_EXPORT_DYNAMIC

dnl Checks for libraries.
dnl Replace `main' with a function in -lcrypt:
AC_CHECK_LIB(crypt, main)
dnl Replace `main' with a function in -ldl:
AC_CHECK_LIB(dl, main)
dnl Replace `main' with a function in -lm:
AC_CHECK_LIB(m, main)
dnl Replace `main' with a function in -lz:
AC_CHECK_LIB(z, main)
dnl Check for libpng
AC_CHECK_LIB(png, png_read_png)
AC_CACHE_SAVE

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h unistd.h readline/readline.h readline/history.h libgen.h png.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(gethostname mkdir rmdir socket strdup strspn strstr strtod strtol basename)
AC_CACHE_SAVE

dnl Readline support
CENTRALLIX_CHECK_READLINE

dnl Dynamically loaded modules
BUILD_DYNAMIC

dnl Centrallix specific tests.
CENTRALLIX_CHECK_CENTRALLIX
AH_TEMPLATE([CENTRALLIX_CONFIG], [Full path of config file])
CENTRALLIX_CONF_FILE
CENTRALLIX_OS_DIR
AC_CACHE_SAVE

dnl Check if -pg is allowed with -fPIC
AH_TEMPLATE([USE_PROFILE], [Define for profiling])
CHECK_PROFILE

dnl GZIP osdriver support.
AH_TEMPLATE([USE_GZIP], [Define for GZIP support])
CENTRALLIX_CHECK_GZIP_OS

dnl HTTP osdriver support.
AH_TEMPLATE([USE_HTTP], [Define for HTTP support])
CENTRALLIX_CHECK_HTTP_OS

dnl MIME osdriver support.
AH_TEMPLATE([USE_MIME], [Define for MIME support])
CENTRALLIX_CHECK_MIME_OS

dnl DBL osdriver support.
AH_TEMPLATE([USE_DBL], [Define for DBL support])
CENTRALLIX_CHECK_DBL_OS

dnl Sybase support.
AH_TEMPLATE([USE_SYBASE], [Define for Sybase support])
CENTRALLIX_CHECK_SYBASE

dnl XML osdriver support.
AH_TEMPLATE([USE_XML], [Define for XML support])
AH_TEMPLATE([USE_LIBXML1], [Define for backward compatibility with libxml1])
CENTRALLIX_CHECK_XML_OS

dnl Mbox support.
AH_TEMPLATE([USE_MBOX], [Define for Mbox support])
CENTRALLIX_CHECK_MBOX_OS

dnl Shell osdriver support.
AH_TEMPLATE([USE_SHELL], [Define for Shell osdriver support])
CENTRALLIX_CHECK_SHELL_OS

dnl NFS netdriver support.
AH_TEMPLATE([USE_NETNFS], [Define for NFS netdriver support])
CENTRALLIX_CHECK_NET_NFS

AC_SUBST(OBJDRIVERMODULES)
AC_SUBST(OBJDRIVERS)
AC_SUBST(NETDRIVERMODULES)
AC_SUBST(NETDRIVERS)
AC_SUBST(STATIC_CFLAGS)
AC_SUBST(STATIC_LIBS)
AC_SUBST(PROFILE)

    
dnl Define WITH_DYNAMIC_LOAD in config.h
if test "$WITH_DYNAMIC_LOAD" = "yes"; then
    AC_DEFINE(WITH_DYNAMIC_LOAD,1,[Use dynamic loading])
fi

AC_OUTPUT(Makefile, echo timestamp > stamp-h)

CENTRALLIX_OUTPUT_SUMMARY
