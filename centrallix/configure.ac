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
AC_PATH_PROG([GZIP], [gzip], [])
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
AC_CHECK_HEADERS(fcntl.h unistd.h readline/readline.h readline/history.h libgen.h png.h stdarg.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(gethostname mkdir rmdir socket strdup strspn strstr strtod strtol basename)
AC_CACHE_SAVE

dnl Readline support
CENTRALLIX_CHECK_READLINE

dnl Dynamically loaded modules
AH_TEMPLATE([CX_STATIC])
AC_DEFINE(CX_STATIC,2)
BUILD_DYNAMIC

dnl Check for different runtime dir than build install dir
CHECK_BUILDDIR
    
dnl Centrallix specific tests.
CENTRALLIX_CHECK_CENTRALLIX
AH_TEMPLATE([CENTRALLIX_CONFIG], [Full path of config file])
CENTRALLIX_CONF_FILE
CENTRALLIX_OS_DIR
AC_CACHE_SAVE

dnl Check for makedepend (typically in XFree86-devel)
CHECK_MAKEDEPEND

dnl Check for OpenSSL
CENTRALLIX_CHECK_OPENSSL

dnl Check for __ctype_b
AH_TEMPLATE([HAVE_CTYPE_B], [Whether __ctype_b is present and usable])
CHECK_CTYPE_B

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

dnl FilePro osdriver support.
AH_TEMPLATE([USE_FP], [Define for FilePro support])
CENTRALLIX_CHECK_FP_OS

dnl Sybase osdriver support.
AH_TEMPLATE([USE_SYBASE], [Define for Sybase support])
CENTRALLIX_CHECK_SYBASE

dnl MySQL osdriver support.
AH_TEMPLATE([USE_MYSQL], [Define for MySQL support])
CENTRALLIX_CHECK_MYSQL

dnl XML osdriver support.
AH_TEMPLATE([USE_XML], [Define for XML support])
AH_TEMPLATE([USE_LIBXML1], [Define for backward compatibility with libxml1])
CENTRALLIX_CHECK_XML_OS

dnl Mbox support.
AH_TEMPLATE([USE_MBOX], [Define for Mbox support])
CENTRALLIX_CHECK_MBOX_OS

dnl POP3 osdriver support
AH_TEMPLATE([USE_POP3], [Define for POP3 support])
CENTRALLIX_CHECK_POP3_OS

dnl Shell osdriver support.
AH_TEMPLATE([USE_SHELL], [Define for Shell osdriver support])
CENTRALLIX_CHECK_SHELL_OS

dnl BerkeleyDB osdriver support.
AH_TEMPLATE([USE_BERK], [Define for BerkeleyDB osdriver support])
CENTRALLIX_CHECK_BERK_OS

dnl SNMP client objectsystem driver support.
dnl AH_TEMPLATE([USE_SNMP], [Define for SNMP Client osdriver support])
dnl CENTRALLIX_CHECK_SNMP_OS

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

dnl Check security hardening facilities
CHECK_HARDENING

dnl Check optimization
CHECK_OPTIMIZE

dnl Check for log method
CHECK_LOGMETHOD

dnl Define WITH_DYNAMIC_LOAD in config.h
if test "$WITH_DYNAMIC_LOAD" = "yes"; then
    AC_DEFINE(WITH_DYNAMIC_LOAD,1,[Use dynamic loading])
fi

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(etc/centrallix.conf.mk:etc/centrallix.conf.in)
AC_CONFIG_COMMANDS([timestamp],[echo timestamp > stamp-h])
AC_OUTPUT

CENTRALLIX_OUTPUT_SUMMARY
