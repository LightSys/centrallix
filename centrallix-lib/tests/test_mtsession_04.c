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
/* Module:	test_mtsession_04.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test the per session error stack: adding messages	*/
/* 		with mssError(), the conversions it understands,	*/
/* 		and reading the stack back.				*/
/************************************************************************/

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

/*** Render the user facing form of the error stack, which leaves out the
 *** module codes.
 ***/
static char* userError(void)
    {
    XString xs;

	xsInit(&xs);
	strtcpy(stack, mssUserError(&xs) ? "" : xs.String, sizeof(stack));
	xsDeInit(&xs);

    return stack;
    }

/*** How many messages the current session is holding.
 ***/
static int errorCount(void)
    {
    pMtSession s = (pMtSession)thGetParam(NULL, "mss");

    return s ? s->ErrList.nItems : -1;
    }

static bool doTest(void)
    {
    bool success = true;
    XString xs;

	/** Outside a session there is no stack to add to, clear, or read. **/
	mssError(1, "MOD", "message");
	success &= EXPECT_EQL(mssClearError(), -1, "%d");
	xsInit(&xs);
	success &= EXPECT_EQL(mssStringError(&xs), -1, "%d");
	success &= EXPECT_EQL(mssUserError(&xs), -1, "%d");
	success &= EXPECT_EQL(xs.Length, 0, "%d");
	xsDeInit(&xs);

	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");

	/** A new session has nothing on its stack. **/
	success &= EXPECT_EQL(errorCount(), 0, "%d");
	success &= EXPECT_STR_EQL(errorStack(), STACK_HEAD);
	success &= EXPECT_STR_EQL(userError(), "");

	/** The first message becomes the whole stack. **/
	mssError(1, "MOD", "first");
	success &= EXPECT_EQL(errorCount(), 1, "%d");
	success &= EXPECT_STR_EQL(errorStack(), STACK_HEAD"--- MOD: first\r\n");

	/** Further messages stack up, and the stack reads newest first. **/
	mssError(0, "MOD2", "second");
	mssError(0, "MOD3", "third");
	success &= EXPECT_EQL(errorCount(), 3, "%d");
	success &= EXPECT_STR_EQL(errorStack(),
		STACK_HEAD"--- MOD3: third\r\n--- MOD2: second\r\n--- MOD: first\r\n");

	/** The user facing form drops the module codes and joins the
	 ** messages with single spaces.
	 **/
	success &= EXPECT_STR_EQL(userError(), "third second first");

	/** Setting clr replaces the stack instead of adding to it. **/
	mssError(1, "MOD", "fresh");
	success &= EXPECT_EQL(errorCount(), 1, "%d");
	success &= EXPECT_STR_EQL(errorStack(), STACK_HEAD"--- MOD: fresh\r\n");

	/*** The message is a printf() format string, so the conversions are
	 *** whatever the C library provides.  An unknown conversion and a
	 *** trailing percent sign are undefined; glibc keeps the percent sign
	 *** and drops the letter after it.
	 ***/
	mssError(1, "FMT",
		"s=%s d=%d c=%c pct=%% unknown=%q trailing=%", "text", -7, 'X');
	success &= EXPECT_STR_EQL(errorStack(),
		STACK_HEAD"--- FMT: s=text d=-7 c=X pct=% unknown=% trailing=%\r\n");

	/** A NULL string argument is spelled out by glibc rather than followed. **/
	mssError(1, "FMT", "%s", (char*)NULL);
	success &= EXPECT_STR_EQL(errorStack(), STACK_HEAD"--- FMT: (null)\r\n");

	/** A message with nothing in it, from a module with no name. **/
	mssError(1, "", "");
	success &= EXPECT_STR_EQL(errorStack(), STACK_HEAD"--- : \r\n");
	success &= EXPECT_STR_EQL(userError(), "");

	/** A message with no conversions at all is passed through. **/
	mssError(1, "MOD", "plain message, no conversions");
	success &= EXPECT_STR_EQL(errorStack(), STACK_HEAD"--- MOD: plain message, no conversions\r\n");

	/** A colon in the message itself does not confuse the user facing
	 ** form, which only drops the module code.
	 **/
	mssError(1, "MOD", "colon: inside");
	success &= EXPECT_STR_EQL(userError(), "colon: inside");

	/** Both forms add to the string they are handed, rather than
	 ** replacing what is already in it.
	 **/
	mssError(1, "MOD", "appended");
	xsInit(&xs);
	xsConcatenate(&xs, "prefix ", -1);
	success &= EXPECT_EQL(check(mssStringError(&xs)), 0, "%d");
	success &= EXPECT_STR_EQL(xs.String, "prefix "STACK_HEAD"--- MOD: appended\r\n");
	xsDeInit(&xs);
	xsInit(&xs);
	xsConcatenate(&xs, "prefix ", -1);
	success &= EXPECT_EQL(check(mssUserError(&xs)), 0, "%d");
	success &= EXPECT_STR_EQL(xs.String, "prefix appended");
	xsDeInit(&xs);

	/** Clearing leaves the session in place with an empty stack. **/
	success &= EXPECT_EQL(check(mssClearError()), 0, "%d");
	success &= EXPECT_EQL(errorCount(), 0, "%d");
	success &= EXPECT_STR_EQL(errorStack(), STACK_HEAD);
	success &= EXPECT_STR_EQL(userError(), "");
	success &= EXPECT_EQL(check(mssClearError()), 0, "%d");
	success &= EXPECT_EQL(errorCount(), 0, "%d");

	/** The stack belongs to the session, so a new session starts empty. **/
	mssError(1, "MOD", "left over");
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");
	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");
	success &= EXPECT_EQL(errorCount(), 0, "%d");
	success &= EXPECT_STR_EQL(errorStack(), STACK_HEAD);
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    long long result;

	*tname = "mtsession-04 Error Stack";

	if (!tmpFileInit(auth_path, sizeof(auth_path))) return -1;
	if (!authFileWriteUser(auth_path, USERNAME, PASSWORD))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	mssInitialize("altpasswd", auth_path, "", 0, "test_mtsession");

	result = loopTest(doTest) * 48ll;

	if (!tmpFileDeInit(auth_path)) return -1;

    return result;
    }

/** Scope cleanup. **/
#undef USERNAME
#undef PASSWORD
#undef STACK_HEAD
#undef STACK_SIZE
