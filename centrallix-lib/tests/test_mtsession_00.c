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
/* 		altpasswd method, reading the session, and ending it.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "test_mtsession.h"
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
	success &= ASSERT_EQL(mssUserName(), NULL, "%p");
	success &= ASSERT_EQL(mssPassword(), NULL, "%p");
	success &= ASSERT_EQL(thGetParam(NULL, "mss"), NULL, "%p");
	success &= ASSERT_EQL(mssEndSession(NULL), -1, "%d");

	/** A wrong password, an unknown user, and a user name with the
	 ** field separator are all refused.  None of them starts a session.
	 **/
	success &= ASSERT_EQL(mssAuthenticate(USERNAME, "wrongpassword", 0), -1, "%d");
	success &= ASSERT_EQL(mssAuthenticate("nosuchuser", PASSWORD, 0), -1, "%d");
	success &= ASSERT_EQL(mssAuthenticate("bad:user", PASSWORD, 0), -1, "%d");
	success &= ASSERT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

	/** The right password starts a session. **/
	success &= ASSERT_EQL(mssAuthenticate(USERNAME, PASSWORD, 0), 0, "%d");
	success &= ASSERT_STR_EQL(mssUserName(), USERNAME);
	success &= ASSERT_STR_EQL(mssPassword(), PASSWORD);

	/** The session is the thread's "mss" parameter. **/
	s = (pMtSession)thGetParam(NULL, "mss");
	success &= ASSERT_NOT_NULL(s);
	if (!s) return false;

	/** An altpasswd session runs as the calling process' own user. **/
	success &= ASSERT_EQL(s->UserID, (int)geteuid(), "%d");
	success &= ASSERT_EQL(s->GroupID, (int)getegid(), "%d");
	success &= ASSERT_EQL(s->LinkCnt, 1, "%d");
	success &= ASSERT_EQL(s->ErrList.nItems, 0, "%d");
	success &= ASSERT_STR_EQL(s->UserName, USERNAME);
	success &= ASSERT_STR_EQL(s->Password, PASSWORD);

	/** Authenticating again replaces the session, and bypass_crypt accepts
	 ** any password.
	 **/
	success &= ASSERT_EQL(mssAuthenticate(USERNAME, "anything", 1), 0, "%d");
	success &= ASSERT_NOT_NULL(thGetParam(NULL, "mss"));
	success &= ASSERT_STR_EQL(mssUserName(), USERNAME);
	success &= ASSERT_STR_EQL(mssPassword(), "anything");

	/** Ending the session detaches it from the thread. **/
	success &= ASSERT_EQL(mssEndSession(NULL), 0, "%d");
	success &= ASSERT_EQL(thGetParam(NULL, "mss"), NULL, "%p");
	success &= ASSERT_EQL(mssUserName(), NULL, "%p");
	success &= ASSERT_EQL(mssPassword(), NULL, "%p");
	success &= ASSERT_EQL(mssEndSession(NULL), -1, "%d");

	/** Ending a session by pointer works the same way. **/
	success &= ASSERT_EQL(mssAuthenticate(USERNAME, PASSWORD, 0), 0, "%d");
	s = (pMtSession)thGetParam(NULL, "mss");
	success &= ASSERT_NOT_NULL(s);
	if (!s) return false;
	success &= ASSERT_EQL(mssEndSession(s), 0, "%d");
	success &= ASSERT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

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
