AC_DEFUN(SHARED_LIBCENTRALLIX,
    [
	AC_MSG_CHECKING(if a shared libCentrallix is desired)
	AC_ARG_ENABLE(shared-libCentrallix,
	    AC_HELP_STRING([--disable-shared-libCentrallix],
		[disable building a shared libCentrallix]
	    ),
	    WITH_SHARED_LIBCENTRALLIX="$enableval",
	    WITH_SHARED_LIBCENTRALLIX="yes"
	)
	if test "$WITH_SHARED_LIBCENTRALLIX" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
	    AC_MSG_CHECKING(if a shared libCentrallix is possible)
	    temp="$CFLAGS"
	    CFLAGS="$CFLAGS -shared"
	    if test "x`$CC -shared 2>&1 | $GREP '-shared'`" = "x"; then
		FINAL_TARGETS="$FINAL_TARGETS libCentrallix.so"
		AC_MSG_RESULT(yes)
	    else
		AC_MSG_RESULT(no)
	    fi
	    CFLAGS="$temp"
	fi
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


dnl check if gcc allows -fPIC and -pg at the same time
AC_DEFUN(CHECK_PROFILE,
    [
    AC_MSG_CHECKING(if -fPIC and -pg can be used at the same time)
    temp="$CFLAGS"
    CFLAGS="-fPIC -pg -Werror"
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

AC_DEFUN(CHECK_MTASK_DEBUG,
    [
	AC_MSG_CHECKING(if mTask debugging is desired)
	AC_ARG_ENABLE(mtask-debug,
	    AC_HELP_STRING([--enable-mtask-debug],
		[enable mTask debugging support]
	    ),
	    WITH_MTASK_DEBUG="$enableval",
	    WITH_MTASK_DEBUG="no"
	)
	if test "$WITH_MTASK_DEBUG" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
	    AC_DEFINE(MTASK_DEBUG,1,[enable mTask debugging])
	fi
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
	    AC_DEFINE(USING_OPTIMIZATION,1,[defined to 1 if -On is being passed to the compiler])
	fi
    fi
    ]
)
	    
	    
