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
	    
	    
