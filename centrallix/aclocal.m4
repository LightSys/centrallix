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

dnl Test for the Centrallix header and library files.
AC_DEFUN(CENTRALLIX_CHECK_CENTRALLIX,
    [
	AC_ARG_WITH(centrallix-inc,
	    AC_HELP_STRING([--with-centrallix-inc=DIR],
		[Location of centrallix-libs include directory (default is PREFIX/include)]
	    ),
	    centrallix_incdir="$withval",
	    centrallix_incdir="$prefix/include"
	)

 	CFLAGS="$CFLAGS -I$centrallix_incdir"
 	CPPFLAGS="$CPPFLAGS -I$centrallix_incdir"
 	AC_CHECK_HEADER(mtask.h, 
	    [],
 	    AC_MSG_ERROR([Please ensure that Centrallix-libs is installed and use --with-centrallix-inc=DIR to specify the path to the header files])
 	)

	AC_ARG_WITH(centrallix-lib,
	    AC_HELP_STRING([--with-centrallix-lib=DIR],
		[Location of centrallix-libs lib directory (default is PREFIX/lib)]
	    ),
	    centrallix_libdir="$withval",
	    centrallix_libdir="$prefix/lib"
	)
	centrallix_libdir="`echo $centrallix_libdir | sed 's|/+|/|g'`"

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
		AC_CHECK_LIB(ct, ct_connect, WITH_SYBASE_CT="yes", WITH_SYBASE_CT="no", -lcomn -lsybtcl -linsck -lintl)
		AC_CHECK_LIB(comn, comn_str_to_ascii, WITH_SYBASE_COMN="yes", WITH_SYBASE_COMN="no", -lintl)
		AC_CHECK_LIB(intl, intl_open, WITH_SYBASE_INTL="yes", WITH_SYBASE_INTL="no")
		AC_CHECK_LIB(sybtcl, netp_init_driver_poll, WITH_SYBASE_SYBTCL="yes", WITH_SYBASE_SYBTCL="no", -lcomn -linsck -lintl)
		AC_CHECK_LIB(cs, cs_time, WITH_SYBASE_CS="yes", WITH_SYBASE_CS="no", -lcomn -lintl)
		AC_CHECK_LIB(insck, bsd_tcp, WITH_SYBASE_INSCK="yes", WITH_SYBASE_INSCK="no", -lcomn -lintl)
		if test "$WITH_SYBASE_CT" = "no" \
		    -o "$WITH_SYBASE_COMN" = "no" \
		    -o "$WITH_SYBASE_INTL" = "no" \
		    -o "$WITH_SYBASE_SYBTCL" = "no" \
		    -o "$WITH_SYBASE_CS" = "no" \
		    -o "$WITH_SYBASE_CS" = "no" \
		    -o "$WITH_SYBASE_INSCK" = "no"; then
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
	    else
		AC_MSG_RESULT(no)
	    fi
	fi

	AC_SUBST(SYBASE_LIBS)
	AC_SUBST(SYBASE_CFLAGS)
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
	    AC_HELP_STRING([--enable-mime],
		[enable MIME support]
	    ),
	    WITH_MIME="$enableval", 
	    WITH_MIME="no"
	)
 
	if test "$WITH_MIME" = "yes"; then
	    AC_DEFINE(USE_MIME)
	    if test "$WITH_DYNAMIC_LOAD" = "yes"; then
		OBJDRIVERMODULES="$OBJDRIVERMODULES objdrv_mime.so"
	    else
		OBJDRIVERS="$OBJDRIVERS objdrv_mime.o"
	    fi
	    AC_MSG_RESULT(yes)
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
	else
	    AC_MSG_RESULT(no)
	fi

	AC_SUBST(XML_CFLAGS)
	AC_SUBST(XML_LIBS)
    ]
)
