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
/* Module:	test_mtsession_00.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test the session lifecycle: authenticating with the	*/
/* 		altpasswd method, reading the session back, and		*/
/* 		ending it.						*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "test_mtsession.h"
#include "check.h"
#include "mtask.h"

/** Tested module. **/
#include "mtsession.h"

#define USERNAME	"testuser"
#define PASSWORD	"testpassword"

static char auth_path[256];

static bool doTest(void)
    {
    bool success = true;
    pMtSession s;

	/** The thread starts out with no session at all. **/
	success &= EXPECT_EQL(mssUserName(), NULL, "%p");
	success &= EXPECT_EQL(mssPassword(), NULL, "%p");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");
	success &= EXPECT_EQL(mssEndSession(NULL), -1, "%d");

	/** A wrong password, an unknown user, and a user name holding the
	 ** field separator are all refused, and none of them starts a session.
	 **/
	success &= EXPECT_EQL(mssAuthenticate(USERNAME, "wrongpassword", 0), -1, "%d");
	success &= EXPECT_EQL(mssAuthenticate("nosuchuser", PASSWORD, 0), -1, "%d");
	success &= EXPECT_EQL(mssAuthenticate("bad:user", PASSWORD, 0), -1, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

	/** The right password starts one. **/
	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");
	success &= EXPECT_STR_EQL(mssUserName(), USERNAME);
	success &= EXPECT_STR_EQL(mssPassword(), PASSWORD);

	/** The session is the thread's "mss" parameter. **/
	s = (pMtSession)thGetParam(NULL, "mss");
	success &= EXPECT_NOT_NULL(s);
	if (!s) return false;

	/** An altpasswd session runs as the calling process' own user. **/
	success &= EXPECT_EQL(s->UserID, (int)geteuid(), "%d");
	success &= EXPECT_EQL(s->GroupID, (int)getegid(), "%d");
	success &= EXPECT_EQL(s->LinkCnt, 1, "%d");
	success &= EXPECT_EQL(s->ErrList.nItems, 0, "%d");
	success &= EXPECT_STR_EQL(s->UserName, USERNAME);
	success &= EXPECT_STR_EQL(s->Password, PASSWORD);

	/** Authenticating again replaces the session, and bypass_crypt takes
	 ** any password at all.
	 **/
	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, "anything", 1)), 0, "%d");
	success &= EXPECT_NOT_NULL(thGetParam(NULL, "mss"));
	success &= EXPECT_STR_EQL(mssUserName(), USERNAME);
	success &= EXPECT_STR_EQL(mssPassword(), "anything");

	/** Ending the session detaches it from the thread. **/
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");
	success &= EXPECT_EQL(mssUserName(), NULL, "%p");
	success &= EXPECT_EQL(mssPassword(), NULL, "%p");
	success &= EXPECT_EQL(mssEndSession(NULL), -1, "%d");

	/** Ending a session by pointer works the same way. **/
	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");
	s = (pMtSession)thGetParam(NULL, "mss");
	success &= EXPECT_NOT_NULL(s);
	if (!s) return false;
	success &= EXPECT_EQL(check(mssEndSession(s)), 0, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

    return success;
    }

long long test(char** tname)
    {
    long long result;

	*tname = "mtsession-00 Session Lifecycle";

	/** Authenticate against a file holding the one user. **/
	if (!tmpFileInit(auth_path, sizeof(auth_path))) return -1;
	if (!authFileWriteUser(auth_path, USERNAME, PASSWORD))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	mssInitialize("altpasswd", auth_path, "", 0, "test_mtsession");

	result = loopTest(doTest) * 31ll;

	if (!tmpFileDeInit(auth_path)) return -1;

    return result;
    }

/** Scope cleanup. **/
#undef USERNAME
#undef PASSWORD
