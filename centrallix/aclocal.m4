dnl Maintain the list of drivers to build (one day this may expand to do a bit
dnl more than this...
AC_DEFUN(CENTRALLIX_ADD_DRIVER,
    [
	DRIVERLIST="$DRIVERLIST $1"
	drivernames="$drivernames|$2"
	AC_SUBST(DRIVERLIST)
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

dnl Test for the Centrallix-LIB header and library files.
AC_DEFUN(CENTRALLIX_CHECK_CENTRALLIX,
    [
	centrallix_incdir="$prefix/include"
	centrallix_libdir="$prefix/lib"
	AC_ARG_WITH(centrallix-inc,
	    AC_HELP_STRING([--with-centrallix-inc=DIR],
		[Location of centrallix-libs include directory (default is PREFIX/include)]
	    ),
	    centrallix_incdir="$withval",
	    centrallix_incdir="$prefix/include"
	)
	if test "$centrallix_incdir" = "NONE/include"; then
	    centrallix_incdir="$ac_default_prefix/include"
	fi

 	CFLAGS="$CFLAGS -I$centrallix_incdir"
 	CPPFLAGS="$CPPFLAGS -I$centrallix_incdir"
 	AC_CHECK_HEADER(mtask.h, 
	    [],
 	    AC_MSG_ERROR([Please ensure that Centrallix-libs is installed and use --with-centrallix-inc=DIR to specify the path to the header files])
 	)
	AC_SUBST(CXINCDIR, $centrallix_incdir)

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
 	AC_CHECK_LIB(Centrallix, 
	    mtInitialize,
	    [],
 	    AC_MSG_ERROR([Please ensure that Centrallix-libs is installed and use --with-centrallix-lib=DIR to specify the path to the library])
 	)
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
 
	AC_ARG_WITH(sybase-inc,
	    AC_HELP_STRING([--with-sybase-inc=PATH],
		[include path for Sybase headers (default is /opt/sybase/include)]
	    ),
	    sybase_incdir="$withval",
	    sybase_incdir="/opt/sybase/include"
	)
 
	AC_ARG_WITH(sybase-lib,
	    AC_HELP_STRING([--with-sybase-lib=PATH],
		[library path for Sybase libraries (default is /opt/sybase/lib)]
	    ),
	    sybase_libdir="$withval",
	    sybase_libdir="/opt/sybase/lib"
	)
 
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
		SYBASE_CFLAGS="-I$sybase_incdir"
		temp=$LIBS
		LIBS="$LIBS -L$sybase_libdir"
		AC_CHECK_LIB(ct, ct_connect, WITH_SYBASE_CT="yes", WITH_SYBASE_CT="no", -lct -lcomn -lsybtcl -linsck -lintl -lcs)
		if test "$WITH_SYBASE_CT" = "no"; then
		    WITH_SYBASE="no"
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
		AC_MSG_RESULT(yes)
		CENTRALLIX_ADD_DRIVER(sybd, [Sybase Database])
	    else
		AC_MSG_RESULT(no)
	    fi
	fi

	AC_SUBST(SYBASE_LIBS)
	AC_SUBST(SYBASE_CFLAGS)
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

	if test "$WITH_GZIP" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)

	    AC_ARG_WITH(zlib,
		AC_HELP_STRING([--with-zlib=PATH],
		    [library path for zlib library (default is /usr/lib)]
		),
		zlib_libdir="$withval",
		zlib_libdir="/usr/lib",
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
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(gzip, GZip)
	else
	    AC_MSG_RESULT(no)
	fi

	AC_SUBST(GZIP_CFLAGS)
	AC_SUBST(GZIP_LIBS)
    ]
)

dnl Test for the HTTP os driver.
AC_DEFUN(CENTRALLIX_CHECK_HTTP_OS,
    [
	AC_MSG_CHECKING(if HTTP support is desired)

	AC_ARG_ENABLE(http,
	    AC_HELP_STRING([--disable-http],
		[disable HTTP support]
	    ),
	    WITH_HTTP="$enableval", 
	    WITH_HTTP="yes"
	)
 
	if test "$WITH_HTTP" = "yes"; then
	    AC_DEFINE(USE_HTTP)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_http.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_http.o"
	    fi
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(http, HTTP)
	else
	    AC_MSG_RESULT(no)
	fi
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
 
	if test "$WITH_MIME" = "yes"; then
	    AC_DEFINE(USE_MIME)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_mime.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_mime.o"
	    fi
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(mime, [MIME encoding])
	else
	    AC_MSG_RESULT(no)
	fi
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
 
	if test "$WITH_DBL" = "yes"; then
	    AC_DEFINE(USE_DBL)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_dbl.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_dbl.o"
	    fi
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(dbl, DBL)
	else
	    AC_MSG_RESULT(no)
	fi
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
 
	if test "$WITH_MBOX" = "yes"; then
	    AC_DEFINE(USE_MBOX)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_mbox.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_mbox.o"
	    fi
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(mbox, mbox)
	else
	    AC_MSG_RESULT(no)
	fi
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
 
	if test "$WITH_SHELL" = "yes"; then
	    AC_DEFINE(USE_SHELL)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_shell.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_shell.o"
	    fi
	    AC_MSG_RESULT(yes)
	    AC_WARN(The shell driver can be _very_ insecure!!!)
	    CENTRALLIX_ADD_DRIVER(shell, shell)
	else
	    AC_MSG_RESULT(no)
	fi
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
			    [library path for xml library (default is /usr/lib)]
			),
			libxml_libdir="$withval",
			libxml_libdir="/usr/lib",
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
			    [library path for xml library (default is /usr/lib)]
			),
			libxml_libdir="$withval",
			libxml_libdir="/usr/lib"
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
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(xml, XML)
	else
	    AC_MSG_RESULT(no)
	fi

	AC_SUBST(XML_CFLAGS)
	AC_SUBST(XML_LIBS)
    ]
)

dnl Test for the SHELL os driver.
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
 
	if test "$WITH_NETNFS" = "yes"; then
	    AC_DEFINE(USE_NETNFS)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		NETDRIVERMODULES="$NETDRIVERMODULES net_nfs.so"
	    else
		NETDRIVERS="$OBJDRIVERS net_nfs.o"
	    fi
	    AC_MSG_RESULT(yes)
	    CENTRALLIX_ADD_DRIVER(nfs,nfs)
	else
	    AC_MSG_RESULT(no)
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

