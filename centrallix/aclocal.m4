dnl Maintain the list of drivers to build (one day this may expand to do a bit
dnl more than this...
AC_DEFUN(CENTRALLIX_ADD_DRIVER,
    [
	DRIVERLIST="$DRIVERLIST $1"
	drivernames="$drivernames|$2"
	AC_SUBST(DRIVERLIST)
    ]
)

dnl Check for the presence of makedepend, without which the build will not
dnl work.  makedepend is typically in an XFree86-devel or xorg-devel package.
AC_DEFUN(CHECK_MAKEDEPEND,
    [
	AC_CHECK_PROG([MAKEDEPEND],[makedepend],[yes],[no])
	if test "$MAKEDEPEND" != "yes"; then
	    AC_MSG_ERROR([*** The "makedepend" utility does not appear to be available on your system.  It is required for building Centrallix and is usually found in the XFree86-devel or xorg-x11-devel packages on RPM-based systems.])
	fi
    ]
)

dnl Test for __ctype_b presence and usability.  Newer glibc versions obliterated
dnl this poor little symbol from being used by older libraries...
AC_DEFUN(CHECK_CTYPE_B,
    [
	has_ctype_b="test"
	AC_MSG_CHECKING(if __ctype_b is present and usable)
	temp="$CFLAGS"
	CFLAGS=""
	AC_TRY_LINK([#include <ctype.h>],[int main() { int x = *(__ctype_b+'a'); return x; }],
	    has_ctype_b="yes",
	    has_ctype_b=""
	)
	if test "$has_ctype_b" = "yes"; then
	    AC_DEFINE(HAVE_CTYPE_B)
	    AC_MSG_RESULT(yes)
	else
	    AC_MSG_RESULT(no)
	fi
    ]
)
	
dnl Test for readline support.  Some versions of readline also require
dnl ncurses, so test for that if necessary.
AC_DEFUN(CENTRALLIX_CHECK_READLINE,
    [
	readline="test"
	AC_CHECK_LIB(readline,
	    readline,
	    [LIBS="$LIBS -lreadline"
	    readline="yes"]
	)
	if test "$readline" = "test"; then
	    unset ac_cv_lib_readline_readline
	    AC_CHECK_LIB(readline,
		readline,
		[LIBS="$LIBS -lreadline -lncurses"
		readline="yes"],,
		-lncurses
	    )
	fi
	if test "$readline" = "test"; then
	    AC_CHECK_FUNCS(readline, readline="yes")
	fi

	if test "$readline" = "yes"; then
	    AC_DEFINE(HAVE_READLINE)
	fi
    ]
)

dnl Test for OpenSSL
AC_DEFUN(CENTRALLIX_CHECK_OPENSSL,
    [
	AC_CHECK_LIB(ssl, SSL_CTX_new, [LIBS="$LIBS -lssl -lcrypto"], AC_MSG_ERROR([Centrallix requires OpenSSL to be installed.]))
	AC_CHECK_HEADER([openssl/md5.h], [], AC_MSG_ERROR([Centrallix requires OpenSSL development header files to be installed.]))
    ]
)

dnl Test for SQLite
AC_DEFUN(CENTRALLIX_CHECK_SQLITE,
    [
	AC_CHECK_LIB(sqlite3, sqlite3_open, [LIBS="$LIBS -lsqlite3"], AC_MSG_ERROR([Centrallix requires SQLite3 to be installed.]))
	AC_CHECK_HEADER([sqlite3.h], [], AC_MSG_ERROR([Centrallix requires SQLite3 development header files to be installed.]))
    ]
)

dnl Test for JSON-C
AC_DEFUN(CENTRALLIX_CHECK_JSONC,
    [
	AC_CHECK_LIB(json-c, json_tokener_new, [LIBS="$LIBS -ljson-c"], AC_MSG_ERROR([Centrallix requires JSON-C to be installed.]))
	AC_CHECK_HEADER([json/json.h], [], AC_MSG_ERROR([Centrallix requires JSON-C development header files to be installed.]))
    ]
)

dnl Test for ncurses
AC_DEFUN(CENTRALLIX_CHECK_NCURSES,
    [
    AC_CHECK_LIB(ncurses, setupterm, [ncurses_found="yes"])
    AC_CHECK_HEADER([curses.h], [curses_h_found="yes"])
    AC_CHECK_HEADER([term.h], [term_h_found="yes"])
    if test "$ncurses_found" = "yes" -a "$curses_h_found" = "yes" -a "$term_h_found" = "yes"; then
       AC_DEFINE(HAVE_NCURSES, 1, [Defined to 1 if you have the ncurses library (-lncurses, curses.h, and term.h).])
       NCURSES_LIBS="-lncurses"
    fi
    ]
)


dnl Test for the Centrallix-LIB header and library files.
AC_DEFUN(CENTRALLIX_CHECK_CENTRALLIX,
    [
	centrallix_incdir="$prefix/include"
	centrallix_libdir="$prefix/lib"
	AC_ARG_WITH(centrallix-inc,
	    AC_HELP_STRING([--with-centrallix-inc=DIR],
		[Location of centrallix-lib include directory (default is PREFIX/include)]
	    ),
	    centrallix_incdir="$withval",
	    centrallix_incdir="$prefix/include"
	)
	if test "$centrallix_incdir" = "NONE/include"; then
	    centrallix_incdir="$ac_default_prefix/include"
	fi

 	CFLAGS="$CFLAGS -I$centrallix_incdir"
 	CPPFLAGS="$CPPFLAGS -I$centrallix_incdir"
 	AC_CHECK_HEADER([cxlib/mtask.h], 
	    [],
 	    AC_MSG_ERROR([Please ensure that Centrallix-lib is installed and use --with-centrallix-inc=DIR to specify the path to the header files])
 	)
	AC_SUBST(CXINCDIR, $centrallix_incdir)

	AC_ARG_WITH(static-libcentrallix,
	    AC_HELP_STRING([--with-static-libcentrallix],
		[Enable to link in libCentrallix statically instead of dynamically]
	    ),
	    go_static="yes",
	    go_static="no"
	)
	AC_ARG_WITH(centrallix-lib,
	    AC_HELP_STRING([--with-centrallix-lib=DIR],
		[Location of centrallix-libs lib directory (default is PREFIX/lib)]
	    ),
	    centrallix_libdir="$withval",
	    centrallix_libdir="$prefix/lib"
	)
	centrallix_libdir="`echo $centrallix_libdir | sed 's|/+|/|g'`"
	if test "$centrallix_libdir" = "NONE/lib"; then
	    centrallix_libdir="$ac_default_prefix/lib"
	fi

	temp=$LIBS
 	LIBS="$LIBS -L$centrallix_libdir"
	if test "$go_static" = "no"; then
	    AC_CHECK_LIB(Centrallix, 
		mtInitialize,
		[],
		AC_MSG_ERROR([Please ensure that Centrallix-libs is installed and use --with-centrallix-lib=DIR to specify the path to the library])
	    )
	else
	    STATIC_LIBS="$STATIC_LIBS $centrallix_libdir/libCentrallix.a"
	fi
	AC_SUBST(CXLIBDIR, $centrallix_libdir)
    ]
)

dnl Define the location of the Centrallix config file
AC_DEFUN(CENTRALLIX_CONF_FILE,
    [
	centrallix_config="$sysconfdir/centrallix.conf"
	if test "$prefix" = "NONE"; then
	    centrallix_config="`echo $centrallix_config | sed -e "s|\\\${prefix}|$ac_default_prefix|"`"
	else
	    centrallix_config="`echo $centrallix_config | sed -e "s|\\\${prefix}|$prefix|"`"
	fi

	centrallix_config=${centrallix_config#$builddir}

	AC_DEFINE_UNQUOTED(CENTRALLIX_CONFIG, 
	    "$centrallix_config", 
	    [Location of the centrallix config file]
	)
    ]
)

dnl Define the location of the Centrallix OS tree
AC_DEFUN(CENTRALLIX_OS_DIR,
    [
	AC_ARG_WITH(centrallix-os,
	    AC_HELP_STRING([--with-centrallix-os=FILE],
		[Full path of centrallix OS directory (default is /var/centrallix/os)]
	    ),
	    centrallix_os="$withval",
	    centrallix_os="/var/centrallix/os"
	)

	AC_SUBST(CXOSDIR, $centrallix_os)
    ]
)

dnl Check for dynamic loading
AC_DEFUN(BUILD_DYNAMIC,
    [
	AC_MSG_CHECKING(if dynamic loading is desired)
	AC_ARG_ENABLE(dynamic-load,
	    AC_HELP_STRING([--disable-dynamic-load],
		[disable dynamic loading]
	    ),
	    WITH_DYNAMIC_LOAD="$enableval",
	    WITH_DYNAMIC_LOAD="auto"
	)
	if test "$WITH_DYNAMIC_LOAD" = "auto"; then
	    AC_MSG_RESULT(autodetecting)
	    AC_MSG_CHECKING(if dynamic loading can be enabled)
	    dnl This isn't the _correct_ way to check, but it works for cygwin
	    if test "$ac_cv_lib_dl_main" = "yes"; then
		WITH_DYNAMIC_LOAD="yes"
	    else
		WITH_DYNAMIC_LOAD="no"
	    fi
	    AC_MSG_RESULT($WITH_DYNAMIC_LOAD)
	else
	    AC_MSG_RESULT($WITH_DYNAMIC_LOAD)
	fi
    ]
)

dnl Test for the Sybase libraries.
AC_DEFUN(CENTRALLIX_CHECK_SYBASE,
    [
	AC_MSG_CHECKING(if Sybase support is desired)

	AC_ARG_ENABLE(sybase,
	    AC_HELP_STRING([--disable-sybase],
		[disable Sybase support]
	    ),
	    WITH_SYBASE="$enableval", 
	    WITH_SYBASE="yes"
	)

	if test "$SYBASE" != ""; then
	    if test "$SYBASE_OCS" != ""; then
		default_sybase_incdir="$SYBASE/$SYBASE_OCS/include"
		default_sybase_libdir="$SYBASE/$SYBASE_OCS/lib"
	    else
		default_sybase_incdir="$SYBASE/include"
		default_sybase_libdir="$SYBASE/lib"
	    fi
	else
	    if test -d "/opt/sybase/"; then
		if test -d /opt/sybase/OCS-*/include -a -d /opt/sybase/OCS-*/lib; then
		    default_sybase_incdir="/opt/sybase/"OCS-*"/include"
		    default_sybase_libdir="/opt/sybase/"OCS-*"/lib"
		else
		    default_sybase_incdir="/opt/sybase/include"
		    default_sybase_libdir="/opt/sybase/lib"
		fi
	    else
		default_sybase_incdir="/opt/sybase/include"
		default_sybase_libdir="/opt/sybase/lib"
	    fi
	fi
 
	AC_ARG_WITH(sybase-inc,
	    AC_HELP_STRING([--with-sybase-inc=PATH],
		[include path for Sybase headers (default is /opt/sybase/include)]
	    ),
	    sybase_incdir="$withval",
	    sybase_incdir="$default_sybase_incdir"
	)
 
	AC_ARG_WITH(sybase-lib,
	    AC_HELP_STRING([--with-sybase-lib=PATH],
		[library path for Sybase libraries (default is /opt/sybase/lib)]
	    ),
	    sybase_libdir="$withval",
	    sybase_libdir="$default_sybase_libdir"
	)

	ENABLE_SYBASE="no"
 
	if test "$WITH_SYBASE" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
 
	    temp=$CPPFLAGS
	    CPPFLAGS="$CPPFLAGS -I$sybase_incdir"
	    AC_CHECK_HEADER(ctpublic.h, 
		WITH_SYBASE="yes",
		WITH_SYBASE="no"
	    )
	    CPPFLAGS="$temp"

	    
	    if test "$WITH_SYBASE" = "yes"; then
		dnl only try compile if we don't have a __ctype_b problem
		if test "$has_ctype_b" = "yes"; then
		    SYBASE_CFLAGS="-I$sybase_incdir"
		    temp=$LIBS
		    LIBS="$LIBS -L$sybase_libdir"
		    AC_CHECK_LIB(ct, ct_connect, WITH_SYBASE_CT="yes", WITH_SYBASE_CT="no", -lct -lcomn -lsybtcl -linsck -lintl -lcs)
		else
		    SYBASE_CFLAGS="-I$sybase_incdir"
		    temp=$LIBS
		    LIBS="$LIBS -L$sybase_libdir"
		    libfile="$sybase_libdir/libct.so"
		    AC_CHECK_FILE($libfile, WITH_SYBASE_CT="yes", WITH_SYBASE_CT="no")
		fi
		if test "$WITH_SYBASE_CT" = "no"; then
		    dnl Dont give up yet.  Could be Sybase 15.0 which uses
		    dnl different library names.
		    SYBASE_CFLAGS="-I$sybase_incdir"
		    temp=$LIBS
		    LIBS="$LIBS -L$sybase_libdir"
		    libfile="$sybase_libdir/libsybct.so"
		    AC_CHECK_FILE($libfile, WITH_SYBASE_CT="yes", WITH_SYBASE_CT="no")
		    if test "$WITH_SYBASE_CT" = "no"; then
			WITH_SYBASE="no"
		    else
		        if test "$PTRSIZE" = "8"; then
			    SYBASE_LIBS="-L$sybase_libdir -lsybct64 -lsybcomn64 -lsybintl64 -lsybtcl64 -lsybcs64"
			else
			    SYBASE_LIBS="-L$sybase_libdir -lsybct -lsybcomn -lsybintl -lsybtcl -lsybcs"
			fi
		    fi
		else
		    SYBASE_LIBS="-L$sybase_libdir -lct -lcomn -lintl -lsybtcl -lcs -linsck"
		fi
		LIBS="$temp"
	    fi

	    AC_MSG_CHECKING(if Sybase support can be enabled)
	    if test "$WITH_SYBASE" = "yes"; then
		AC_DEFINE(USE_SYBASE)
		if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		    OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_sybase.so"
		else
		    STATIC_CFLAGS="$STATIC_CFLAGS $SYBASE_CFLAGS"
		    STATIC_LIBS="$STATIC_LIBS $SYBASE_LIBS"
		    OBJDRIVERS="$OBJDRIVERS objdrv_sybase.o"
		fi
		ENABLE_SYBASE="yes"
		AC_MSG_RESULT(yes)
		CENTRALLIX_ADD_DRIVER(sybd, [Sybase Database])
	    else
		AC_MSG_RESULT(no)
	    fi
	fi

	AC_SUBST(ENABLE_SYBASE)
	AC_SUBST(SYBASE_LIBS)
	AC_SUBST(SYBASE_CFLAGS)
    ]
)

dnl Test for the MySQL libraries.
AC_DEFUN(CENTRALLIX_CHECK_MYSQL,
    [
	AC_MSG_CHECKING(if MySQL support is desired)

	AC_ARG_ENABLE(mysql,
	    AC_HELP_STRING([--disable-mysql],
		[disable MySQL support]
	    ),
	    WITH_MYSQL="$enableval", 
	    WITH_MYSQL="yes"
	)

	default_mysql_incdir="/usr/include/mysql"
	default_mysql_libdir="$LINKDIR/mysql"
	
	AC_ARG_WITH(mysql-inc,
	    AC_HELP_STRING([--with-mysql-inc=PATH],
		[include path for MySQL headers (default is /usr/include/mysql)]
	    ),
	    mysql_incdir="$withval",
	    mysql_incdir="$default_mysql_incdir"
	)
 
	AC_ARG_WITH(mysql-lib,
	    AC_HELP_STRING([--with-mysql-lib=PATH],
		[library path for MySQL libraries (default is $LINKDIR/mysql)]
	    ),
	    mysql_libdir="$withval",
	    mysql_libdir="$default_mysql_libdir"
	)

	ENABLE_MYSQL="no"
 
	if test "$WITH_MYSQL" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
 
	    temp=$CPPFLAGS
	    CPPFLAGS="$CPPFLAGS -I$mysql_incdir -L$mysql_libdir -lmysqlclient -lz"
	    AC_CHECK_HEADER(mysql.h, 
		WITH_MYSQL="yes",
		WITH_MYSQL="no"
	    )
	    CPPFLAGS="$temp"

	    
	    AC_MSG_CHECKING(if MySQL support can be enabled)
	    if test "$WITH_MYSQL" = "yes"; then
		AC_DEFINE(USE_MYSQL)
		if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		    OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_mysql.so"
		else
		    STATIC_CFLAGS="$STATIC_CFLAGS $MYSQL_CFLAGS"
		    STATIC_LIBS="$STATIC_LIBS $MYSQL_LIBS"
		    OBJDRIVERS="$OBJDRIVERS objdrv_mysql.o"
		fi
		ENABLE_MYSQL="yes"
		AC_MSG_RESULT(yes)
		CENTRALLIX_ADD_DRIVER(mysd, [MySQL Database])
	    else
		AC_MSG_RESULT(no)
	    fi
	fi

	MYSQL_CFLAGS="-I$mysql_incdir"
	MYSQL_LIBS="-L$mysql_libdir -lmysqlclient"
	LIBS="$LIBS -L$mysql_libdir"

	AC_SUBST(ENABLE_MYSQL)
	AC_SUBST(MYSQL_LIBS)
	AC_SUBST(MYSQL_CFLAGS)
    ]
)

dnl Test for the GZIP os driver.
AC_DEFUN(CENTRALLIX_CHECK_GZIP_OS,
    [
	AC_MSG_CHECKING(if gzip support is desired)

	AC_ARG_ENABLE(gzip,
	    AC_HELP_STRING([--disable-gzip],
		[disable gzip support]
	    ),
	    WITH_GZIP="$enableval", 
	    WITH_GZIP="yes"
	)

	ENABLE_GZIP="no"

	if test "$WITH_GZIP" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)

	    AC_ARG_WITH(zlib,
		AC_HELP_STRING([--with-zlib=PATH],
		    [library path for zlib library (default is $LINKDIR)]
		),
		zlib_libdir="$withval",
		zlib_libdir="$LINKDIR",
	    )

	    AC_ARG_WITH(zlib-inc,
		AC_HELP_STRING([--with-zlib-inc=PATH],
		    [include path for zlib headers (default is /usr/include)]
		),
		zlib_incdir="-I$withval",
		zlib_incdir="/usr/include"
	    )

	    temp=$CPPFLAGS
	    CPPFLAGS="$CPPFLAGS $zlib_incdir"
	    tempC=$CFLAGS
	    CFLAGS="$CFLAGS $zlib_incdir"
	    AC_CHECK_HEADER(zlib.h, 
		WITH_GZIP="yes",
		WITH_GZIP="no"
	    )
	    CPPFLAGS="$temp"
	    CFLAGS="$tempC"

	    if test "$WITH_GZIP" = "yes"; then
		GZIP_CFLAGS="-I$zlib_incdir"
		temp=$LIBS
		LIBS="$LIBS -L$zlib_libdir -lz"
		AC_CHECK_LIB(z, gzread, WITH_GZIP_ZLIB="yes", WITH_GZIP_ZLIB="no", -lz)
		if test "$WITH_GZIP_ZLIB" = "no"; then
		    WITH_GZIP="no"
		else
		    GZIP_LIBS="-L$zlib_libdir $zlib_lib"
		fi
		LIBS="$temp"
	    fi
	fi

	AC_MSG_CHECKING(if GZIP support can be enabled)
	if test "$WITH_GZIP" = "yes"; then
	    AC_DEFINE(USE_GZIP)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_gzip.so"
	    else
		STATIC_CFLAGS="$STATIC_CFLAGS $GZIP_CFLAGS"
		STATIC_LIBS="$STATIC_LIBS $GZIP_LIBS"
		OBJDRIVERS="$OBJDRIVERS objdrv_gzip.o"
	    fi

	    ENABLE_GZIP="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(gzip, GZip)
	else
	    AC_MSG_RESULT(no)
	fi

	AC_SUBST(ENABLE_GZIP)
	AC_SUBST(GZIP_CFLAGS)
	AC_SUBST(GZIP_LIBS)
    ]
)

dnl Test for the HTTP os driver.
AC_DEFUN(CENTRALLIX_CHECK_HTTP_OS,
    [
	AC_MSG_CHECKING(if HTTP client support is desired)

	AC_ARG_ENABLE(http,
	    AC_HELP_STRING([--disable-http],
		[disable HTTP client support]
	    ),
	    WITH_HTTP="$enableval", 
	    WITH_HTTP="yes"
	)

	ENABLE_HTTP="no"
 
	if test "$WITH_HTTP" = "yes"; then
	    AC_DEFINE(USE_HTTP)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_http.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_http.o"
	    fi

	    ENABLE_HTTP="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(http, HTTP)
	else
	    AC_MSG_RESULT(no)
	fi

	AC_SUBST(ENABLE_HTTP)
    ]
)

dnl Test for the MIME os driver.
AC_DEFUN(CENTRALLIX_CHECK_MIME_OS,
    [
	AC_MSG_CHECKING(if MIME support is desired)

	AC_ARG_ENABLE(mime,
	    AC_HELP_STRING([--disable-mime],
		[disable MIME support]
	    ),
	    WITH_MIME="$enableval", 
	    WITH_MIME="yes"
	)

	ENABLE_MIME="no"
 
	if test "$WITH_MIME" = "yes"; then
	    AC_DEFINE(USE_MIME)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_mime.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_mime.o"
	    fi
	    ENABLE_MIME="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(mime, [MIME encoding])
	else
	    AC_MSG_RESULT(no)
	fi
	AC_SUBST(ENABLE_MIME)
    ]
)

dnl Test for the POP3 os driver.
AC_DEFUN(CENTRALLIX_CHECK_POP3_OS,
    [
        AC_MSG_CHECKING(if POP3 support is desired)

        AC_ARG_ENABLE(pop3,
            AC_HELP_STRING([--disable-pop3],
                [disable POP3 support]
            ),
            WITH_POP3="$enableval", 
            WITH_POP3="yes"
        )

        ENABLE_POP3="no"
 
        if test "$WITH_POP3" = "yes"; then
            AC_DEFINE(USE_POP3)
            if test "$WITH_DYNAMIC_LOAD" = "yes"; then
                OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_pop3_v3.so"
            else
                OBJDRIVERS="$OBJDRIVERS objdrv_pop3_v3.o"
            fi

            ENABLE_POP3="yes"
            AC_MSG_RESULT(yes)
            CENTRALLIX_ADD_DRIVER(pop3, POP3)
        else
            AC_MSG_RESULT(no)
        fi

        AC_SUBST(ENABLE_POP3)
    ]
)


dnl Test for the FilePro osdriver.
AC_DEFUN(CENTRALLIX_CHECK_FP_OS,
    [
	AC_MSG_CHECKING(if FilePro support is desired)

	AC_ARG_ENABLE(filepro,
	    AC_HELP_STRING([--enable-filepro],
		[enable FilePro support]
	    ),
	    WITH_FP="$enableval", 
	    WITH_FP="no"
	)

	ENABLE_FP="no"
 
	if test "$WITH_FP" = "yes"; then
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		AC_DEFINE(USE_FP)
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_fp.so"
	    else
		AC_DEFINE(USE_FP,CX_STATIC)
		OBJDRIVERS="$OBJDRIVERS objdrv_fp.o"
	    fi
	    ENABLE_FP="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(fp, FilePro)
	else
	    if test "$WITH_FP" = "static"; then
		AC_DEFINE(USE_FP,CX_STATIC)
		OBJDRIVERS="$OBJDRIVERS objdrv_fp.o"
		ENABLE_FP="yes"
		AC_MSG_RESULT(yes)
		CENTRALLIX_ADD_DRIVER(fp, FilePro)
	    else
		AC_MSG_RESULT(no)
	    fi
	fi
	AC_SUBST(ENABLE_FP)
    ]
)


dnl Test for the DBL os driver.
AC_DEFUN(CENTRALLIX_CHECK_DBL_OS,
    [
	AC_MSG_CHECKING(if DBL support is desired)

	AC_ARG_ENABLE(dbl,
	    AC_HELP_STRING([--enable-dbl],
		[enable DBL support]
	    ),
	    WITH_DBL="$enableval", 
	    WITH_DBL="no"
	)

	ENABLE_DBL="no"
 
	if test "$WITH_DBL" = "yes"; then
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		AC_DEFINE(USE_DBL)
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_dbl.so"
	    else
		AC_DEFINE(USE_DBL,CX_STATIC)
		OBJDRIVERS="$OBJDRIVERS objdrv_dbl.o"
	    fi
	    ENABLE_DBL="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(dbl, DBL)
	else
	    if test "$WITH_DBL" = "static"; then
		AC_DEFINE(USE_DBL,CX_STATIC)
		OBJDRIVERS="$OBJDRIVERS objdrv_dbl.o"
		ENABLE_DBL="yes"
		AC_MSG_RESULT(yes)
		CENTRALLIX_ADD_DRIVER(dbl, DBL)
	    else
		AC_MSG_RESULT(no)
	    fi
	fi
	AC_SUBST(ENABLE_DBL)
    ]
)

dnl Test for the Mbox os driver.
AC_DEFUN(CENTRALLIX_CHECK_MBOX_OS,
    [
	AC_MSG_CHECKING(if MBOX support is desired)

	AC_ARG_ENABLE(mbox,
	    AC_HELP_STRING([--disable-mbox],
		[disable mbox support]
	    ),
	    WITH_MBOX="$enableval", 
	    WITH_MBOX="yes"
	)
 
	ENABLE_MBOX="no"
	
	if test "$WITH_MBOX" = "yes"; then
	    AC_DEFINE(USE_MBOX)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_mbox.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_mbox.o"
	    fi
	    ENABLE_MBOX="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(mbox, mbox)
	else
	    AC_MSG_RESULT(no)
	fi
	AC_SUBST(ENABLE_MBOX)
    ]
)

dnl Test for the SHELL os driver.
AC_DEFUN(CENTRALLIX_CHECK_SHELL_OS,
    [
	AC_MSG_CHECKING(if shell osdriver support is desired)

	AC_ARG_ENABLE(shell-os,
	    AC_HELP_STRING([--enable-shell-os],
		[enable shell osdriver support]
	    ),
	    WITH_SHELL="$enableval", 
	    WITH_SHELL="no"
	)

	ENABLE_SHELL="no"
 
	if test "$WITH_SHELL" = "yes"; then
	    AC_DEFINE(USE_SHELL)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_shell.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_shell.o"
	    fi
	    AC_MSG_RESULT(yes)
	    AC_WARN(The shell driver can be _very_ insecure!!!)
	    AC_WARN(It will be built, but *not* enabled by default)
	    CENTRALLIX_ADD_DRIVER(shell, shell)
	else
	    AC_MSG_RESULT(no)
	fi

	AC_SUBST(ENABLE_SHELL)
    ]
)

dnl Test for the SNMP os driver.
dnl AC_DEFUN(CENTRALLIX_CHECK_SNMP_OS,
dnl     [
dnl 	AC_MSG_CHECKING(if SNMP osdriver support is desired)
dnl 
dnl 	AC_ARG_ENABLE(snmp-os,
dnl 	    AC_HELP_STRING([--enable-snmp-os],
dnl 		[enable SNMP osdriver support]
dnl 	    ),
dnl 	    WITH_SNMP="$enableval",
dnl 	    WITH_SNMP="no"
dnl 	)
dnl 
dnl 	ENABLE_SNMP="no"
dnl 
dnl 	if test "$WITH_SNMP" = "yes"; then
dnl 	    AC_MSG_RESULT(yes)
dnl 	    dnl check for alternate locations for includes
dnl 	    AC_ARG_WITH(ucd-snmp,
dnl 		AC_HELP_STRING([--with-ucd-snmp=PATH],
dnl 		    [library path for ucd-snmp library (default is $LINKDIR)]
dnl 		),
dnl 		snmp_libdir="$withval",
dnl 		snmp_libdir="$LINKDIR",
dnl 	    )
dnl 
dnl 	    dnl check for alternate locations for libs
dnl 	    AC_ARG_WITH(ucd-snmp-inc,
dnl 		AC_HELP_STRING([--with-ucd-snmp-inc=PATH],
dnl 		    [include path for ucd-snmp headers (default is /usr/include)]
dnl 		),
dnl 		snmp_incdir="-I$withval",
dnl 		snmp_incdir="/usr/include"
dnl 	    )
dnl 
dnl 	    dnl make sure the headers are there.
dnl 	    temp=$CPPFLAGS
dnl 	    CPPFLAGS="$CPPFLAGS $snmp_incdir"
dnl 	    tempC=$CFLAGS
dnl 	    CFLAGS="$CFLAGS $snmp_incdir"
dnl 	    AC_CHECK_HEADER(ucd-snmp/snmp_api.h, 
dnl 		WITH_SNMP="yes",
dnl 		WITH_SNMP="no",
dnl 		[#include <sys/types.h>
dnl 		 #include <netinet/in.h>
dnl 		 #include <sys/time.h>
dnl 		 #include <ucd-snmp/asn1.h>
dnl 		 #include <ucd-snmp/snmp_impl.h>
dnl 		 #include <ucd-snmp/snmp.h>
dnl 		]
dnl 	    )
dnl 	    CPPFLAGS="$temp"
dnl 	    CFLAGS="$tempC"
dnl 
dnl 	    dnl if we're still ok, make sure the library is there and useable.
dnl 	    if test "$WITH_SNMP" = "yes"; then
dnl 		SNMP_CFLAGS="-I$snmp_incdir"
dnl 		temp=$LIBS
dnl 		LIBS="$LIBS -L$snmp_libdir -lsnmp -lcrypto"
dnl 		AC_CHECK_LIB(snmp, snmp_sess_init, WITH_SNMP_LIBSNMP="yes", WITH_SNMP_LIBSNMP="no", -lsnmp)
dnl 		if test "$WITH_SNMP_LIBSNMP" = "no"; then
dnl 		    WITH_SNMP="no"
dnl 		else
dnl 		    SNMP_LIBS="-L$snmp_libdir -lsnmp -lcrypto"
dnl 		fi
dnl 		LIBS="$temp"
dnl 	    fi
dnl 	else
dnl 	    AC_MSG_RESULT(no)
dnl 	fi
dnl 
dnl 	dnl ok, if snmp is wanted and available, build it.
dnl 	AC_MSG_CHECKING(if we can enable SNMP support)
dnl 	if test "$WITH_SNMP" = "yes"; then
dnl 	    AC_DEFINE(USE_SNMP)
dnl 	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
dnl 		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_snmp.so"
dnl 	    else
dnl 		OBJDRIVERS="$OBJDRIVERS objdrv_snmp.o"
dnl 	    fi
dnl 	    ENABLE_SNMP="yes"
dnl 	    AC_MSG_RESULT(yes)
dnl 	    CENTRALLIX_ADD_DRIVER(snmp, snmp)
dnl 	else
dnl 	    AC_MSG_RESULT(no)
dnl 	fi
dnl 
dnl 	AC_SUBST(ENABLE_SNMP)
dnl 	AC_SUBST(SNMP_CFLAGS)
dnl 	AC_SUBST(SNMP_LIBS)
dnl     ]
dnl )

dnl Test for the BerkeleyDB os driver.
AC_DEFUN(CENTRALLIX_CHECK_BERK_OS,
    [
	AC_MSG_CHECKING(if BerkeleyDB osdriver support is desired)

	AC_ARG_ENABLE(berkeleydb-os,
	    AC_HELP_STRING([--enable-berkeleydb-os],
		[enable berkeleydb osdriver support]
	    ),
	    WITH_BERK="$enableval", 
	    WITH_BERK="no"
	)

	ENABLE_BERK="no"
 
	if test "$WITH_BERK" = "yes"; then
	    AC_DEFINE(USE_BERK)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_berk.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_berk.o"
	    fi
	    ENABLE_BERK="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(berk, berk)
	else
	    AC_MSG_RESULT(no)
	fi

	AC_SUBST(ENABLE_BERK)
    ]
)

dnl Test for the XML os driver.
AC_DEFUN(CENTRALLIX_CHECK_XML_OS,
    [
	AC_MSG_CHECKING(if xml support is desired)

	AC_ARG_ENABLE(xml,
	    AC_HELP_STRING([--disable-xml],
		[disable xml support]
	    ),
	    WITH_XML="$enableval", 
	    WITH_XML="yes"
	)

	ENABLE_XML="no"

	if test "$WITH_XML" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)

	    AC_ARG_ENABLE(libxml2,
		AC_HELP_STRING([--disable-libxml2],
		    [ignore libxml2 even if installed]
		),
		USE_XML2="$enableval",
		USE_XML2="yes"
	    )

	    if test "$USE_XML2" = "no"; then
		AC_CHECK_PROG(xmlconfig, 
		    xml-config,
		    xml-config,
		    no)
	    else
		AC_CHECK_PROGS(xmlconfig, 
		    xml2-config xml-config,
		    no)
	    fi

	    if test "$xmlconfig" = "no"; then
		WITH_XML="neither"
	    else
		if test "$xmlconfig" = "xml2-config"; then

		    AC_ARG_WITH(libxml,
			AC_HELP_STRING([--with-libxml=PATH],
			    [library path for xml library (default is $LINKDIR)]
			),
			libxml_libdir="$withval",
			libxml_libdir="$LINKDIR",
		    )

		    libxml_lib="`xml2-config --libs 2>/dev/null`"

		    AC_ARG_WITH(libxml-inc,
			AC_HELP_STRING([--with-libxml-inc=PATH],
			    [include path for libxml headers (default is /usr/include)]
			),
			libxml_incdir="-I$withval",
			libxml_incdir="`xml2-config --cflags 2>/dev/null`"
		    )
 
		    temp=$CPPFLAGS
		    CPPFLAGS="$CPPFLAGS $libxml_incdir"
		    tempC=$CFLAGS
		    CFLAGS="$CFLAGS $libxml_incdir"
		    AC_CHECK_HEADER(libxml/parser.h, 
			WITH_XML="yes",
			WITH_XML="no"
		    )
		    CPPFLAGS="$temp"
		    CFLAGS="$tempC"

		    if test "$WITH_XML" = "yes"; then
			XML_CFLAGS="$libxml_incdir"
			temp=$LIBS
			LIBS="$LIBS -L$libxml_libdir $libxml_lib"
			AC_CHECK_LIB(xml2, xmlParseFile, WITH_XML_XML="yes", WITH_XML_XML="no", $libxml_lib)
			if test "$WITH_XML_XML" = "no"; then
			    WITH_XML="no"
			else
			    XML_LIBS="-L$libxml_libdir $libxml_lib"
			fi
			LIBS="$temp"
		    fi
		else
		    AC_ARG_WITH(libxml,
			AC_HELP_STRING([--with-libxml=PATH],
			    [library path for xml library (default is $LINKDIR)]
			),
			libxml_libdir="$withval",
			libxml_libdir="$LINKDIR"
		    )

		    libxml_lib="`xml-config --libs 2>/dev/null`"

		    AC_ARG_WITH(libxml-inc,
			AC_HELP_STRING([--with-libxml-inc=PATH],
			    [include path for libxml headers (default is /usr/include/gnome-xml)]
			),
			libxml_incdir="-I$withval",
			libxml_incdir="`xml-config --cflags 2>/dev/null`"
		    )
 
		    temp=$CPPFLAGS
		    CPPFLAGS="$CPPFLAGS $libxml_incdir"
		    tempC=$CFLAGS
		    CFLAGS="$CFLAGS $libxml_incdir"
		    AC_CHECK_HEADER(parser.h, 
			WITH_XML="yes",
			WITH_XML="no"
		    )
		    CPPFLAGS="$temp"
		    CFLAGS="$tempC"

		    if test "$WITH_XML" = "yes"; then
			XML_CFLAGS="$libxml_incdir"
			temp=$LIBS
			LIBS="$LIBS -L$libxml_libdir $libxml_lib"
			AC_CHECK_LIB(xml, xmlParseFile, WITH_XML_XML="yes", WITH_XML_XML="no", $libxml_lib)
			if test "$WITH_XML_XML" = "no"; then
			    WITH_XML="no"
			else
			    XML_LIBS="-L$libxml_libdir $libxml_lib"
			fi
			LIBS="$temp"
		    fi
		    AC_DEFINE(USE_LIBXML1) 
		fi
	    fi
	fi

	AC_MSG_CHECKING(if XML support can be enabled)
	if test "$WITH_XML" = "yes"; then
	    AC_DEFINE(USE_XML)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_xml.so"
	    else
		STATIC_CFLAGS="$STATIC_CFLAGS $XML_CFLAGS"
		STATIC_LIBS="$STATIC_LIBS $XML_LIBS"
		OBJDRIVERS="$OBJDRIVERS objdrv_xml.o"
	    fi
	    ENABLE_XML="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(xml, XML)
	else
	    AC_MSG_RESULT(no)
	fi

	AC_SUBST(ENABLE_XML)
	AC_SUBST(XML_CFLAGS)
	AC_SUBST(XML_LIBS)
    ]
)

dnl Test for LDAP netdriver.
AC_DEFUN(CENTRALLIX_CHECK_NET_LDAP,
    [
	AC_MSG_CHECKING(if LDAP netdriver support is desired)

	AC_ARG_ENABLE(net-ldap,
	    AC_HELP_STRING([--enable-net-ldap],
		[enable LDAP netdriver support]
	    ),
	    WITH_NETLDAP="$enableval",
	    WITH_NETLDAP="no"
	)

	default_lber_incdir="$prefix/include"
	default_lber_libdir="$prefix/lib"
	
	AC_ARG_WITH(lber-inc,
	    AC_HELP_STRING([--with-lber-inc=PATH],
		[include path for OpenLDAP LBER headers (default is $default_lber_incdir)]
	    ),
	    lber_incdir="$withval",
	    lber_incdir="$default_lber_incdir"
	)
 
	AC_ARG_WITH(lber-lib,
	    AC_HELP_STRING([--with-lber-lib=PATH],
		[library path for OpenLDAP LBER libraries (default is $default_lber_libdir)]
	    ),
	    lber_libdir="$withval",
	    lber_libdir="$default_lber_libdir"
	)

	ENABLE_NETLDAP="no"

	if test "$WITH_NETLDAP" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
 
	    temp=$CPPFLAGS
	    CPPFLAGS="$CPPFLAGS -I$lber_incdir -L$lber_libdir -llber"
	    AC_CHECK_HEADER(lber.h, 
		WITH_NETLDAP="yes",
		WITH_NETLDAP="no"
	    )
	    CPPFLAGS="$temp"
	    
	    AC_MSG_CHECKING(if LDAP netdriver support can be enabled)
	    if test "$WITH_NETLDAP" = "yes"; then
		AC_DEFINE(USE_NETLDAP)
		if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		    NETDRIVERMODULES="$NETDRIVERMODULES net_ldap.so"
		else
		    STATIC_CFLAGS="$STATIC_CFLAGS $NETLDAP_CFLAGS"
		    STATIC_LIBS="$STATIC_LIBS $NETLDAP_LIBS"
		    NETDRIVERS="$NETDRIVERS net_ldap.o"
		fi
		ENABLE_NETLDAP="yes"
		AC_MSG_RESULT(yes)
		CENTRALLIX_ADD_DRIVER(ldap,ldap)
	    else
		AC_MSG_RESULT(no)
	    fi
	fi

	NETLDAP_CFLAGS="-I$lber_incdir"
	NETLDAP_LIBS="-L$lber_libdir -llber"
	LIBS="$LIBS -L$lber_libdir"

	AC_SUBST(ENABLE_NETLDAP)
	AC_SUBST(NETLDAP_LIBS)
	AC_SUBST(NETLDAP_CFLAGS)
    ]
)

dnl Test for the nfs netdriver.
AC_DEFUN(CENTRALLIX_CHECK_NET_NFS,
    [
	AC_MSG_CHECKING(if NFS netdriver support is desired)

	AC_ARG_ENABLE(net-nfs,
	    AC_HELP_STRING([--enable-net-nfs],
		[enable NFS netdriver support]
	    ),
	    WITH_NETNFS="$enableval", 
	    WITH_NETNFS="no"
	)

	ENABLE_NETNFS="no"
 
	if test "$WITH_NETNFS" = "yes"; then
	    AC_DEFINE(USE_NETNFS)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		NETDRIVERMODULES="$NETDRIVERMODULES net_nfs.so"
	    else
		NETDRIVERS="$OBJDRIVERS net_nfs.o"
	    fi
	    ENABLE_NETNFS="yes"
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(nfs,nfs)
	else
	    AC_MSG_RESULT(no)
	fi
	AC_SUBST(ENABLE_NETNFS)
    ]
)

dnl check if test coverage metering is desired
AC_DEFUN(CHECK_COVERAGE,
    [
	AC_MSG_CHECKING(if test coverage metering is desired)
	AC_ARG_ENABLE(coverage,
	    AC_HELP_STRING([--enable-coverage],
		[enable test coverage metering]
	    ),
	    COVERAGE="-fprofile-arcs -ftest-coverage",
	    COVERAGE=""
	)
	if test "$COVERAGE" = ""; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
	fi
    ]
)


dnl check if gcc allows -fPIC and -pg at the same time
AC_DEFUN(CHECK_PROFILE,
    [
	AC_MSG_CHECKING(if -fPIC and -pg can be used at the same time)
	temp="$CFLAGS"
	CFLAGS="-fPIC -pg -Werror"
	dnl AC_TRY_COMPILE([int main(){return 0;}],
	AC_TRY_COMPILE(,,
	    PROFILE="-pg",
	    PROFILE=""
	)
	if test "$PROFILE" = "-pg"; then
	    AC_MSG_RESULT(yes)
	else
	    AC_MSG_RESULT(no)
	fi
	CFLAGS="$temp"

	AC_MSG_CHECKING(if call graph profiling is desired)
	AC_ARG_ENABLE(profile,
	    AC_HELP_STRING([--enable-profile],
		[enable call graph profiler]
	    ),
	    PROFILE="$PROFILE",
	    PROFILE=""
	)
	if test "$PROFILE" = ""; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
	fi
    ]
)

dnl check if ld allows -export-dynamic
AC_DEFUN(CHECK_EXPORT_DYNAMIC,
    [
	AC_MSG_CHECKING(if -export-dynamic can be given to ld)
	temp="$CFLAGS"
	CFLAGS="-Wl,-export-dynamic"
	AC_TRY_RUN([int main(){return 0;}],
	    EXPORT_DYNAMIC="-export-dynamic",
	    EXPORT_DYNAMIC="" 
	)
	if test "$EXPORT_DYNAMIC" = "-export-dynamic"; then
	    EXPORT_DYNAMIC=",$EXPORT_DYNAMIC"
	    AC_MSG_RESULT(yes)
	else
	    AC_MSG_RESULT(no)
	fi
	CFLAGS="$temp"
	AC_SUBST(EXPORT_DYNAMIC)
    ]
)

AC_DEFUN(CHECK_HARDENING,
    [
	AC_MSG_CHECKING(if application-level hardening desired)
	AC_ARG_ENABLE(hardening,
	    AC_HELP_STRING([--enable-hardening],
		[enable support for application-level security hardening; large performance hit]
	    ),
	    WITH_HARDENING="$enableval",
	    WITH_HARDENING="no"
	)
	if test "$WITH_HARDENING" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
	    DEFS="$DEFS -DCXLIB_SECH"
	    AC_DEFINE(CXLIB_SECURITY_HARDENING,1,[defined to 1 if -DCXLIB_SECH is being passed to the compiler])
	fi
    ]
)

dnl check if optimization should be done
dnl this check must follow the hardening check in configure.ac.
dnl The USING_blahblah defines are to force a recompile if those options
dnl are changed.  They are not otherwise used in practice.
AC_DEFUN(CHECK_OPTIMIZE,
    [
    AC_MSG_CHECKING(if this build should be optimized)
    AC_ARG_ENABLE(optimization,
	AC_HELP_STRING([--enable-optimization],
	    [turn on build optimization; incompatible with the --enable-hardening option]
	),
	WITH_OPTIMIZATION="$enableval",
	WITH_OPTIMIZATION="no"
    )
    if test "$WITH_OPTIMIZATION" = "no"; then
	AC_MSG_RESULT(no)
	DEFS="$DEFS -DDBMAGIC"
	AC_DEFINE(USING_DBMAGIC,1,[defined to 1 if -DDBMAGIC is being passed to the compiler; enabled unless optimization is in use])
    else
	if test "$WITH_HARDENING" = "yes"; then
	    AC_MSG_ERROR([Optimization and security hardening are mutually exclusive; please at most specify one or the other but not both])
	else
	    AC_MSG_RESULT(yes)
	    CFLAGS="$CFLAGS -O2"
	    PROFILE=""
	    AC_DEFINE(USING_OPTIMIZATION,1,[defined to 1 if -On is being passed to the compiler])
	fi
    fi
    ]
)

dnl print a nice summary for the end of the configure script
AC_DEFUN(CENTRALLIX_OUTPUT_SUMMARY,
    [
	echo
	echo
	echo "Configuration Summary:"
	echo
	echo "               Install to: $prefix"
	echo "Centrallix libraries from: $centrallix_libdir"
	if test "$WITH_DYNAMIC_LOAD" = "yes"; then
	    echo "      Using shared libraries"
	else
	    echo "      Not using shared libraries"
	fi
	if test "$WITH_HARDENING" = "yes"; then
	    echo "      Using hardening"
	else
	    echo "      Not using hardening"
	fi
	if test "$WITH_OPTIMIZATION" = "yes"; then
	    echo "      Using optimization"
	else
	    echo "      Not using optimization"
	fi
	echo
	echo -n "The following drivers will be built:"
	temp=$IFS
	IFS="|"
	for i in $drivernames; do
	    echo "    $i"
	done
	IFS=$temp
    ]
)

dnl Allow logging method to be configured with this script.
AC_DEFUN(CHECK_LOGMETHOD,
    [
	AC_MSG_CHECKING(for logmethod setting)
	AC_ARG_WITH(logmethod,
	    AC_HELP_STRING([--with-logmethod=method],
		[Error logging method, stdout or syslog, default stdout]
	    ),
	    logmethod="$withval",
	    logmethod="stdout"
	)
	AC_MSG_RESULT($logmethod)
	AC_SUBST(LOGMETHOD, $logmethod)
    ]
)

dnl Check for runtime dirs that differ from install dirs.  Needed
dnl for RPM builds to work correctly.
AC_DEFUN(CHECK_BUILDDIR,
    [
	AC_ARG_WITH(builddir,
	    AC_HELP_STRING([--with-builddir=dir],
		[If build dir is not same as deploy dir, set build dir here]
	    ),
	    builddir="$withval",
	    builddir=""
	)
	builddir=${builddir%/}
	AC_SUBST(BUILDDIR, $builddir)
    ]
)


dnl These are pkg-config macros (imported from /usr/share/aclocal/pkg.m4)
# PKG_PROG_PKG_CONFIG([MIN-VERSION])
# ----------------------------------
AC_DEFUN([PKG_PROG_PKG_CONFIG],
[m4_pattern_forbid([^_?PKG_[A-Z_]+$])
m4_pattern_allow([^PKG_CONFIG(_(PATH|LIBDIR|SYSROOT_DIR|ALLOW_SYSTEM_(CFLAGS|LIBS)))?$])
m4_pattern_allow([^PKG_CONFIG_(DISABLE_UNINSTALLED|TOP_BUILD_DIR|DEBUG_SPEW)$])
AC_ARG_VAR([PKG_CONFIG], [path to pkg-config utility])
AC_ARG_VAR([PKG_CONFIG_PATH], [directories to add to pkg-config's search path])
AC_ARG_VAR([PKG_CONFIG_LIBDIR], [path overriding pkg-config's built-in search path])

if test "x$ac_cv_env_PKG_CONFIG_set" != "xset"; then
	AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
fi
if test -n "$PKG_CONFIG"; then
	_pkg_min_version=m4_default([$1], [0.9.0])
	AC_MSG_CHECKING([pkg-config is at least version $_pkg_min_version])
	if $PKG_CONFIG --atleast-pkgconfig-version $_pkg_min_version; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		PKG_CONFIG=""
	fi
fi[]dnl
])# PKG_PROG_PKG_CONFIG

# PKG_CHECK_EXISTS(MODULES, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# Check to see whether a particular set of modules exists.  Similar
# to PKG_CHECK_MODULES(), but does not set variables or print errors.
#
# Please remember that m4 expands AC_REQUIRE([PKG_PROG_PKG_CONFIG])
# only at the first occurence in configure.ac, so if the first place
# it's called might be skipped (such as if it is within an "if", you
# have to call PKG_CHECK_EXISTS manually
# --------------------------------------------------------------
AC_DEFUN([PKG_CHECK_EXISTS],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
if test -n "$PKG_CONFIG" && \
    AC_RUN_LOG([$PKG_CONFIG --exists --print-errors "$1"]); then
  m4_default([$2], [:])
m4_ifvaln([$3], [else
  $3])dnl
fi])

# _PKG_CONFIG([VARIABLE], [COMMAND], [MODULES])
# ---------------------------------------------
m4_define([_PKG_CONFIG],
[if test -n "$$1"; then
    pkg_cv_[]$1="$$1"
 elif test -n "$PKG_CONFIG"; then
    PKG_CHECK_EXISTS([$3],
                     [pkg_cv_[]$1=`$PKG_CONFIG --[]$2 "$3" 2>/dev/null`
		      test "x$?" != "x0" && pkg_failed=yes ],
		     [pkg_failed=yes])
 else
    pkg_failed=untried
fi[]dnl
])# _PKG_CONFIG

# _PKG_SHORT_ERRORS_SUPPORTED
# -----------------------------
AC_DEFUN([_PKG_SHORT_ERRORS_SUPPORTED],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])
if $PKG_CONFIG --atleast-pkgconfig-version 0.20; then
        _pkg_short_errors_supported=yes
else
        _pkg_short_errors_supported=no
fi[]dnl
])# _PKG_SHORT_ERRORS_SUPPORTED


# PKG_CHECK_MODULES(VARIABLE-PREFIX, MODULES, [ACTION-IF-FOUND],
# [ACTION-IF-NOT-FOUND])
#
#
# Note that if there is a possibility the first call to
# PKG_CHECK_MODULES might not happen, you should be sure to include an
# explicit call to PKG_PROG_PKG_CONFIG in your configure.ac
#
#
# --------------------------------------------------------------
AC_DEFUN([PKG_CHECK_MODULES],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
AC_ARG_VAR([$1][_CFLAGS], [C compiler flags for $1, overriding pkg-config])dnl
AC_ARG_VAR([$1][_LIBS], [linker flags for $1, overriding pkg-config])dnl

pkg_failed=no
AC_MSG_CHECKING([for $1])

_PKG_CONFIG([$1][_CFLAGS], [cflags], [$2])
_PKG_CONFIG([$1][_LIBS], [libs], [$2])

m4_define([_PKG_TEXT], [Alternatively, you may set the environment variables $1[]_CFLAGS
and $1[]_LIBS to avoid the need to call pkg-config.
See the pkg-config man page for more details.])

if test $pkg_failed = yes; then
   	AC_MSG_RESULT([no])
        _PKG_SHORT_ERRORS_SUPPORTED
        if test $_pkg_short_errors_supported = yes; then
	        $1[]_PKG_ERRORS=`$PKG_CONFIG --short-errors --print-errors --cflags --libs "$2" 2>&1`
        else 
	        $1[]_PKG_ERRORS=`$PKG_CONFIG --print-errors --cflags --libs "$2" 2>&1`
        fi
	# Put the nasty error message in config.log where it belongs
	echo "$$1[]_PKG_ERRORS" >&AS_MESSAGE_LOG_FD

	m4_default([$4], [AC_MSG_ERROR(
[Package requirements ($2) were not met:

$$1_PKG_ERRORS

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

_PKG_TEXT])[]dnl
        ])
elif test $pkg_failed = untried; then
     	AC_MSG_RESULT([no])
	m4_default([$4], [AC_MSG_FAILURE(
[The pkg-config script could not be found or is too old.  Make sure it
is in your PATH or set the PKG_CONFIG environment variable to the full
path to pkg-config.

_PKG_TEXT

To get pkg-config, see <http://pkg-config.freedesktop.org/>.])[]dnl
        ])
else
	$1[]_CFLAGS=$pkg_cv_[]$1[]_CFLAGS
	$1[]_LIBS=$pkg_cv_[]$1[]_LIBS
        AC_MSG_RESULT([yes])
	$3
fi[]dnl
])# PKG_CHECK_MODULES


# PKG_INSTALLDIR(DIRECTORY)
# -------------------------
# Substitutes the variable pkgconfigdir as the location where a module
# should install pkg-config .pc files. By default the directory is
# $libdir/pkgconfig, but the default can be changed by passing
# DIRECTORY. The user can override through the --with-pkgconfigdir
# parameter.
AC_DEFUN([PKG_INSTALLDIR],
[m4_pushdef([pkg_default], [m4_default([$1], ['${libdir}/pkgconfig'])])
m4_pushdef([pkg_description],
    [pkg-config installation directory @<:@]pkg_default[@:>@])
AC_ARG_WITH([pkgconfigdir],
    [AS_HELP_STRING([--with-pkgconfigdir], pkg_description)],,
    [with_pkgconfigdir=]pkg_default)
AC_SUBST([pkgconfigdir], [$with_pkgconfigdir])
m4_popdef([pkg_default])
m4_popdef([pkg_description])
]) dnl PKG_INSTALLDIR


# PKG_NOARCH_INSTALLDIR(DIRECTORY)
# -------------------------
# Substitutes the variable noarch_pkgconfigdir as the location where a
# module should install arch-independent pkg-config .pc files. By
# default the directory is $datadir/pkgconfig, but the default can be
# changed by passing DIRECTORY. The user can override through the
# --with-noarch-pkgconfigdir parameter.
AC_DEFUN([PKG_NOARCH_INSTALLDIR],
[m4_pushdef([pkg_default], [m4_default([$1], ['${datadir}/pkgconfig'])])
m4_pushdef([pkg_description],
    [pkg-config arch-independent installation directory @<:@]pkg_default[@:>@])
AC_ARG_WITH([noarch-pkgconfigdir],
    [AS_HELP_STRING([--with-noarch-pkgconfigdir], pkg_description)],,
    [with_noarch_pkgconfigdir=]pkg_default)
AC_SUBST([noarch_pkgconfigdir], [$with_noarch_pkgconfigdir])
m4_popdef([pkg_default])
m4_popdef([pkg_description])
]) dnl PKG_NOARCH_INSTALLDIR


dnl Check for svg image support (cairo/rsvg)
AC_DEFUN(CENTRALLIX_CHECK_RSVG,
    [
        PKG_CHECK_MODULES(librsvg, librsvg-2.0, 
                          WITH_RSVG="yes",
                          WITH_RSVG="no")

	AC_CHECK_LIB(cairo, cairo_svg_surface_set_document_unit, HAVE_CAIRO="yes", HAVE_CAIRO="no")

        if test "$WITH_RSVG" = "yes"; then
	    if test "$HAVE_CAIRO" = "yes"; then
		CFLAGS="$CFLAGS $librsvg_CFLAGS"
		LIBS="$LIBS $librsvg_LIBS"
		AC_DEFINE([HAVE_LIBRSVG], [1], [Define to 1 if librsvg is present.])
		AC_DEFINE([HAVE_RSVG_H], [1], [Define to 1 if <rsvg.h> is present.])
	    else
		AC_MSG_WARN([recent Cairo was not found -- svg support will be disabled])
		WITH_RSVG="no"
	    fi
        else
            AC_MSG_WARN([librsvg was not found -- svg support will be disabled])
        fi
    ]
)

dnl Check for MathGL versions
AC_DEFUN(CENTRALLIX_CHECK_MATHGL,
    [
	AC_CHECK_HEADER(mgl/mgl_c.h,
	    AC_DEFINE([HAVE_MGL1], [1], [Define to 1 if MathGL 1.x is present.]),
	    WITH_MGL1="no"
	)
	AC_CHECK_HEADER(mgl2/mgl.h,
	    AC_DEFINE([HAVE_MGL2], [1], [Define to 1 if MathGL 2.x is present.]),
	    WITH_MGL2="no",
	    [#include <stdbool.h>
	    #include <complex.h>
	    ]
	)
    ]
)
