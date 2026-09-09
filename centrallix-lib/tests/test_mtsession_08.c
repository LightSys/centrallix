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
/* Module:	test_mtsession_08.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test mssLinkSession() and mssUnlinkSession(), which	*/
/* 		count the holders of a session and end it when the	*/
/* 		last one lets go.  They are called here directly:	*/
/* 		mtask calls them for a thread it creates or kills,	*/
/* 		but creating threads under valgrind reports errors	*/
/* 		from mtask itself unless it is built for valgrind.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>

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
    pMtSession current;

	/** A new session is held by the one thread that started it. **/
	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");
	s = (pMtSession)thGetParam(NULL, "mss");
	success &= EXPECT_NOT_NULL(s);
	if (!s) return false;
	success &= EXPECT_EQL(s->LinkCnt, 1, "%d");

	/** Each link counts, and each unlink gives one back. **/
	success &= EXPECT_EQL(check(mssLinkSession(s)), 0, "%d");
	success &= EXPECT_EQL(s->LinkCnt, 2, "%d");
	success &= EXPECT_EQL(check(mssLinkSession(s)), 0, "%d");
	success &= EXPECT_EQL(s->LinkCnt, 3, "%d");
	success &= EXPECT_EQL(check(mssUnlinkSession(s)), 0, "%d");
	success &= EXPECT_EQL(s->LinkCnt, 2, "%d");
	success &= EXPECT_EQL(check(mssUnlinkSession(s)), 0, "%d");
	success &= EXPECT_EQL(s->LinkCnt, 1, "%d");

	/** While links remain, the session is still the thread's. **/
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), (void*)s, "%p");
	success &= EXPECT_STR_EQL(mssUserName(), USERNAME);

	/** The last unlink ends the session. **/
	success &= EXPECT_EQL(check(mssUnlinkSession(s)), 0, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");
	success &= EXPECT_EQL(mssUserName(), NULL, "%p");

	/*** A session held past the thread that started it outlives that
	 *** thread's next login, and ending it by pointer leaves the thread's
	 *** own session alone.
	 ***/
	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");
	s = (pMtSession)thGetParam(NULL, "mss");
	success &= EXPECT_NOT_NULL(s);
	if (!s) return false;
	success &= EXPECT_EQL(check(mssLinkSession(s)), 0, "%d");
	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");
	current = (pMtSession)thGetParam(NULL, "mss");
	success &= EXPECT_NOT_NULL(current);
	if (!current) return false;
	success &= EXPECT_EQL(s->LinkCnt, 1, "%d");
	success &= EXPECT_EQL(check(mssEndSession(s)), 0, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), (void*)current, "%p");
	success &= EXPECT_STR_EQL(mssUserName(), USERNAME);

	/** The thread's own session ends as usual. **/
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");
	success &= EXPECT_EQL(mssEndSession(NULL), -1, "%d");

    return success;
    }

long long test(char** tname)
    {
    long long result;

	*tname = "mtsession-08 Session Link Counting";

	if (!tmpFileInit(auth_path, sizeof(auth_path))) return -1;
	if (!authFileWriteUser(auth_path, USERNAME, PASSWORD))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	mssInitialize("altpasswd", auth_path, "", 0, "test_mtsession");

	result = loopTest(doTest) * 27ll;

	if (!tmpFileDeInit(auth_path)) return -1;

    return result;
    }

/** Scope cleanup. **/
#undef USERNAME
#undef PASSWORD
