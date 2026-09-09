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
/* Module:	test_mtsession_03.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test the system auth method, which authenticates	*/
/* 		against the passwd and shadow files, and the		*/
/* 		handling of an auth method the module does not know.	*/
/************************************************************************/

#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "check.h"
#include "mtask.h"

/** Tested module. **/
#include "mtsession.h"

/** A user name the system will not have. **/
#define NO_SUCH_USER	"no_such_user_mtsession"

static struct passwd* pw;

static bool doTest(void)
    {
    bool success = true;
    pMtSession s;

	/*** The system method takes its user and group from the passwd file.
	 *** bypass_crypt is the only way in without knowing the password.
	 ***
	 *** What the session does to the thread's security context is out of
	 *** reach here: mtask only lets a thread running as root set those,
	 *** and running this as root would have initgroups() change the
	 *** groups of the whole process.
	 ***/
	mssInitialize("system", "", "", 0, "test_mtsession");
	success &= EXPECT_EQL(check(mssAuthenticate(pw->pw_name, "unused", 1)), 0, "%d");
	s = (pMtSession)thGetParam(NULL, "mss");
	success &= EXPECT_NOT_NULL(s);
	if (!s) return false;
	success &= EXPECT_EQL(s->UserID, (int)pw->pw_uid, "%d");
	success &= EXPECT_EQL(s->GroupID, (int)pw->pw_gid, "%d");
	success &= EXPECT_STR_EQL(s->UserName, pw->pw_name);
	success &= EXPECT_STR_EQL(s->Password, "unused");
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");

	/** A user the system does not know is refused, bypass or not. **/
	success &= EXPECT_EQL(mssAuthenticate(NO_SUCH_USER, "unused", 1), -1, "%d");
	success &= EXPECT_EQL(mssAuthenticate(NO_SUCH_USER, "unused", 0), -1, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

	/** A real user with the wrong password is refused as well.  The test
	 ** does not know the real one, so only this direction is checked.
	 **/
	success &= EXPECT_EQL(mssAuthenticate(pw->pw_name, "wrongpassword", 0), -1, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

	/** A user name holding the auth file field separator is refused
	 ** before the auth method gets a look at it.
	 **/
	success &= EXPECT_EQL(mssAuthenticate("bad:user", "unused", 1), -1, "%d");

	/** An auth method the module does not implement lets nobody in. **/
	mssInitialize("nosuchmethod", "", "", 0, "test_mtsession");
	success &= EXPECT_EQL(mssAuthenticate(pw->pw_name, "unused", 1), -1, "%d");
	success &= EXPECT_EQL(mssAuthenticate(pw->pw_name, "unused", 0), -1, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

	/** Neither does an empty one. **/
	mssInitialize("", "", "", 0, "test_mtsession");
	success &= EXPECT_EQL(mssAuthenticate(pw->pw_name, "unused", 1), -1, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

    return success;
    }

long long test(char** tname)
    {
	*tname = "mtsession-03 System Auth Method";

	/** The one account whose passwd entry the test can count on. **/
	pw = getpwuid(geteuid());
	if (!pw)
	    {
	    printFail("getpwuid() found no entry for the current user");
	    return -1;
	    }

    return loopTest(doTest) * 18ll;
    }

/** Scope cleanup. **/
#undef NO_SUCH_USER
