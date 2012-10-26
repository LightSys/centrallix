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
	    if test "x`$CC -shared 2>&1 | $GREP -- '-shared'`" = "x"; then
		FINAL_TARGETS="$FINAL_TARGETS libCentrallix.so.$CXLIBVER libStParse.so.$CXLIBVER libCentrallix.so.$CXLIBVER_GEN libStParse.so.$CXLIBVER_GEN libCentrallix.so libStParse.so"
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

AC_DEFUN(CHECK_DEBUG,
    [
	AC_MSG_CHECKING(if debugging should be enabled)
	AC_ARG_ENABLE(debugging,
	    AC_HELP_STRING([--enable-debugging],
		[enable debugging support (-g enabled regardless, unless optimization also turned on)]
	    ),
	    WITH_DEBUGGING="$enableval",
	    WITH_DEBUGGING="no"
	)
	if test "$WITH_DEBUGGING" = "no"; then
	    AC_MSG_RESULT(no)
	else
	    AC_MSG_RESULT(yes)
	    AC_DEFINE(NMMALLOC_DEBUG,1,[enable newmalloc subsystem debugging])
	    AC_DEFINE(NMMALLOC_PROFILING,1,[enable newmalloc subsystem profiling])
	fi
    ]
)

AC_DEFUN(CHECK_HARDENING,
    [
	AC_MSG_CHECKING(if application-level hardening desired)
	AC_ARG_WITH(hardening,
	    AC_HELP_STRING([--with-hardening],
		[enable support for application-level security hardening.   Can have a large performance hit.  Values: none, low, medium, high; default: none]
	    ),
	    WITH_HARDENING="$withval",
	    WITH_HARDENING="none"
	)
	if test "$WITH_HARDENING" = "none"; then
	    AC_MSG_RESULT(none)
	    if test "$WITH_DEBUGGING" = "no"; then
		CFLAGS="$CFLAGS -DNDEBUG"
	    else
		CFLAGS="$CFLAGS"
	    fi
	elif test "$WITH_HARDENING" = "low"; then
	    AC_MSG_RESULT([low: magic number checking])
	    if test "$WITH_DEBUGGING" = "no"; then
		CFLAGS="$CFLAGS -DNDEBUG -DDBMAGIC"
	    else
		CFLAGS="$CFLAGS -DDBMAGIC"
	    fi
	    AC_DEFINE(USING_DBMAGIC,1,[defined to 1 if -DDBMAGIC is being passed to the compiler; enabled unless optimization is in use])
	elif test "$WITH_HARDENING" = "medium"; then
	    AC_MSG_RESULT([medium: magic number checking and assertions])
	    CFLAGS="$CFLAGS -DDBMAGIC"
	    AC_DEFINE(USING_DBMAGIC,1,[defined to 1 if -DDBMAGIC is being passed to the compiler; enabled unless optimization is in use])
	elif test "$WITH_HARDENING" = "high"; then
	    AC_MSG_RESULT([high: magic number checking, assertions, and data structure verification])
	    CFLAGS="$CFLAGS -DDBMAGIC -DCXLIB_SECH"
	    AC_DEFINE(USING_DBMAGIC,1,[defined to 1 if -DDBMAGIC is being passed to the compiler; enabled unless optimization is in use])
	    AC_DEFINE(CXLIB_SECURITY_HARDENING,1,[defined to 1 if -DCXLIB_SECH is being passed to the compiler])
	else
	    AC_MSG_ERROR([invalid setting for --with-hardening; values=none,low,medium,high])
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
	    [turn on build optimization; incompatible with the --with-hardening=high option]
	),
	WITH_OPTIMIZATION="$enableval",
	WITH_OPTIMIZATION="no"
    )
    if test "$WITH_OPTIMIZATION" = "no"; then
	AC_MSG_RESULT(no)
dnl	DEFS="$DEFS -DDBMAGIC"
dnl	AC_DEFINE(USING_DBMAGIC,1,[defined to 1 if -DDBMAGIC is being passed to the compiler; enabled unless optimization is in use])
    else
	if test "$WITH_HARDENING" = "high"; then
	    AC_MSG_ERROR([Optimization and security hardening are mutually exclusive; please at most specify one or the other but not both])
	else
	    AC_MSG_RESULT(yes)
	    PROFILE=""
	    CFLAGS="$CFLAGS -O3"
	    AC_DEFINE(USING_OPTIMIZATION,1,[defined to 1 if -On is being passed to the compiler])
	fi
    fi
    ]
)

dnl Check for Valgrind integration.  Valgrind is a tool used to check for memory
dnl management and pointer related problems.  But it does NOT get along with
dnl MTASK under normal conditions.  Here we try to fix that.
AC_DEFUN(CHECK_VALGRIND,
    [
    AC_MSG_CHECKING(if valgrind integration is desired)
    AC_ARG_ENABLE(valgrind-integration,
	AC_HELP_STRING([--enable-valgrind-integration],
	    [Valgrind integration allows libCentrallix programs to be debugged using the Valgrind tool; incompatible with optimization]
	),
	WITH_VALGRIND="$enableval",
	WITH_VALGRIND="no"
    )
    if test "$WITH_VALGRIND" = "yes"; then
	if test "$WITH_OPTIMIZATION" = "yes"; then
	    AC_MSG_ERROR([Valgrind integration and Optimization are mutually exclusive; please at most specify one or the other but not both])
	else
	    AC_CHECK_HEADER([valgrind/valgrind.h],
		[], 
		AC_MSG_ERROR([Header file valgrind/valgrind.h not found.  Please install Valgrind if you want to build with Valgrind integration.])
	    )
	    AC_DEFINE(USING_VALGRIND,1,[defined to 1 if valgrind integration is enabled])
	fi
    fi
    ]
)
	    
	    
