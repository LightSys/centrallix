/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2026 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_mtsession_05.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test mssErrorErrno(), which stacks a message with	*/
/* 		the text of the current errno attached.			*/
/************************************************************************/

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "test_mtsession.h"
#include "check.h"
#include "mtask.h"
#include "strtcpy.h"
#include "xstring.h"

/** Tested module. **/
#include "mtsession.h"

#define USERNAME	"testuser"
#define PASSWORD	"testpassword"

/** Every error stack starts with this line. **/
#define STACK_HEAD	"ERROR - Session By Username ["USERNAME"]\r\n"

/** Big enough for any error stack this test builds. **/
#define STACK_SIZE	1024

static char auth_path[256];
static char stack[STACK_SIZE];
static char expected[STACK_SIZE];

/*** Render the error stack of the current session.  An absent session gives
 *** an empty string, which no real stack can produce.
 ***/
static char* errorStack(void)
    {
    XString xs;

	xsInit(&xs);
	strtcpy(stack, mssStringError(&xs) ? "" : xs.String, sizeof(stack));
	xsDeInit(&xs);

    return stack;
    }

/*** Build the stack a single message with the given errno should produce.
 ***/
static char* expectStack(char* message, int en)
    {

	snprintf(expected, sizeof(expected), STACK_HEAD"--- %s (%s)\r\n", message, strerror(en));

    return expected;
    }

/*** How many messages the current session is holding.
 ***/
static int errorCount(void)
    {
    pMtSession s = (pMtSession)thGetParam(NULL, "mss");

    return s ? s->ErrList.nItems : -1;
    }

/*** The calls here are not wrapped in check(), which clears errno before
 *** running what it is given, and errno is exactly what is under test.
 ***/
static bool doTest(void)
    {
    bool success = true;
    int saved_stdout;

	/*** Outside a session there is nowhere to put the message, so it is
	 *** logged instead; what gets logged is test 09's business, so stdout
	 *** is put away for the call.
	 ***/
	if (!quietStart(&saved_stdout)) return false;
	errno = ENOENT;
	mssErrorErrno(1, "MOD", "message");
	if (!quietEnd(saved_stdout)) return false;
	success &= EXPECT_EQL(errorCount(), -1, "%d");

	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");

	/** The message carries the text of the current errno. **/
	errno = ENOENT;
	mssErrorErrno(1, "MOD", "could not open it");
	success &= EXPECT_EQL(errorCount(), 1, "%d");
	success &= EXPECT_STR_EQL(errorStack(), expectStack("MOD: could not open it", ENOENT));

	/** A different errno gives different text. **/
	errno = EACCES;
	mssErrorErrno(1, "MOD", "could not open it");
	success &= EXPECT_STR_EQL(errorStack(), expectStack("MOD: could not open it", EACCES));

	/** Even a zero errno has text of its own. **/
	errno = 0;
	mssErrorErrno(1, "MOD", "nothing went wrong");
	success &= EXPECT_STR_EQL(errorStack(), expectStack("MOD: nothing went wrong", 0));

	/*** The message is a printf() format string, so the conversions are
	 *** whatever the C library provides.  An unknown conversion and a
	 *** trailing percent sign are undefined; glibc keeps the percent sign
	 *** and drops the letter after it.
	 ***/
	errno = ENOENT;
	mssErrorErrno(1, "FMT",
		"s=%s d=%d pct=%% unknown=%q trailing=%", "text", -7);
	success &= EXPECT_STR_EQL(errorStack(),
		expectStack("FMT: s=text d=-7 pct=% unknown=% trailing=%", ENOENT));

	/** A NULL string argument is spelled out by glibc rather than followed. **/
	errno = ENOENT;
	mssErrorErrno(1, "FMT", "%s", (char*)NULL);
	success &= EXPECT_STR_EQL(errorStack(), expectStack("FMT: (null)", ENOENT));

	/** Clearing is honored, and the messages share one stack with the
	 ** ones mssError() adds.
	 **/
	errno = ENOENT;
	mssErrorErrno(0, "MOD", "second");
	success &= EXPECT_EQL(errorCount(), 2, "%d");
	mssError(0, "MOD", "third");
	success &= EXPECT_EQL(errorCount(), 3, "%d");
	errno = ENOENT;
	mssErrorErrno(1, "MOD", "only one left");
	success &= EXPECT_EQL(errorCount(), 1, "%d");
	success &= EXPECT_STR_EQL(errorStack(), expectStack("MOD: only one left", ENOENT));

	/** The user facing form drops the module code as usual. **/
	XString xs;
	xsInit(&xs);
	success &= EXPECT_EQL(check(mssUserError(&xs)), 0, "%d");
	snprintf(expected, sizeof(expected), "only one left (%s)", strerror(ENOENT));
	success &= EXPECT_STR_EQL(xs.String, expected);
	xsDeInit(&xs);

	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    long long result;

	*tname = "mtsession-05 Error Stack With Errno";

	if (!tmpFileInit(auth_path, sizeof(auth_path))) return -1;
	if (!authFileWriteUser(auth_path, USERNAME, PASSWORD))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	mssInitialize("altpasswd", auth_path, "", 0, "test_mtsession");

	result = loopTest(doTest) * 23ll;

	if (!tmpFileDeInit(auth_path)) return -1;

    return result;
    }

/** Scope cleanup. **/
#undef USERNAME
#undef PASSWORD
#undef STACK_HEAD
#undef STACK_SIZE
