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
/* Module:	test_mtsession_02.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test how the altpasswd method reads its auth file:	*/
/* 		which entries match a user, and which are refused.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

/** One attempt to authenticate against a given auth file. **/
typedef struct
    {
    char*	Entries;	/** File contents; each %s takes a credential. **/
    char*	UserName;
    char*	Password;
    int		Bypass;
    int		Expected;	/** Return value mssAuthenticate() should give. **/
    }
    AuthCase;

/** No entry may ask for more than three credentials. **/
static AuthCase cases[] =
    {
	/** The user's own entry, with the password it was built from. **/
	{"testuser:%s\n", USERNAME, PASSWORD, 0, 0},

	/** Any other password is refused, unless the caller says the
	 ** credentials were already checked elsewhere.
	 **/
	{"testuser:%s\n", USERNAME, "wrongpassword", 0, -1},
	{"testuser:%s\n", USERNAME, "", 0, -1},
	{"testuser:%s\n", USERNAME, "wrongpassword", 1, 0},

	/** A user with no entry in the file cannot get in either way. **/
	{"testuser:%s\n", "otheruser", PASSWORD, 0, -1},
	{"testuser:%s\n", "otheruser", PASSWORD, 1, -1},
	{"", USERNAME, PASSWORD, 0, -1},

	/*** An entry with no credential is refused rather than crashing, and
	 *** so is one whose credential crypt() cannot make sense of.  These
	 *** are the entries for which crypt() hands back nothing at all.
	 ***/
	{"testuser:\n", USERNAME, PASSWORD, 0, -1},
	{"testuser:x\n", USERNAME, PASSWORD, 0, -1},
	{"testuser:not a credential\n", USERNAME, PASSWORD, 0, -1},

	/** The last line needs no newline of its own. **/
	{"testuser:%s", USERNAME, PASSWORD, 0, 0},

	/** A name that merely starts with the name being looked up is not a
	 ** match, and neither is one that differs in case or in spacing.
	 **/
	{"testuserx:%s\ntestuser:%s\n", USERNAME, PASSWORD, 0, 0},
	{"testuserx:%s\n", USERNAME, PASSWORD, 0, -1},
	{"TestUser:%s\n", USERNAME, PASSWORD, 0, -1},
	{" testuser:%s\n", USERNAME, PASSWORD, 0, -1},

	/** The first matching entry is the one that counts. **/
	{"testuser:%s\ntestuser:x\n", USERNAME, PASSWORD, 0, 0},
	{"testuser:x\ntestuser:%s\n", USERNAME, PASSWORD, 0, -1},

	/** Lines without a name and separator are passed over. **/
	{"nonsense\ntestuser\n:justacolon\ntestuser:%s\n", USERNAME, PASSWORD, 0, 0},

	/** An empty user name matches an entry whose name field is empty,
	 ** and still has to have the password right.
	 **/
	{":%s\n", "", PASSWORD, 0, 0},
	{":%s\n", "", "wrongpassword", 0, -1},
	{"testuser:%s\n", "", PASSWORD, 0, -1},
    };

#define CASE_COUNT	((int)(sizeof(cases) / sizeof(AuthCase)))

static char auth_path[256];
static char cred[CRED_SIZE];

/*** The auth file is rewritten for every case, so it is held open rather
 *** than opened each time.
 ***/
static int auth_fd = -1;

/*** Replace the contents of the open auth file.
 ***
 *** @param contents The bytes to write.
 *** @param length How many of them.
 *** @returns true if successful, false otherwise.
 ***/
static bool authFileSet(char* contents, int length)
    {

	if (ftruncate(auth_fd, 0) || pwrite(auth_fd, contents, length, 0) != length)
	    {
	    perror("authFileSet: could not write the auth file");
	    return false;
	    }

    return true;
    }

static bool doTest(void)
    {
    bool success = true;
    char contents[AUTH_FILE_SIZE];
    AuthCase* c;
    int i;
    int rval;
    int bypass_rval;
    int saved_stdout;

	for (i = 0; i < CASE_COUNT; i++)
	    {
	    c = &cases[i];

	    /** Fill in the credential wherever the case asked for one. **/
	    snprintf(contents, sizeof(contents), c->Entries, cred, cred, cred);
	    if (!authFileSet(contents, strlen(contents))) return false;

	    /** Authenticate, and expect a session only where one was won. **/
	    success &= EXPECT_EQL(mssAuthenticate(c->UserName, c->Password, c->Bypass), c->Expected, "%d");
	    success &= EXPECT_EQL(thGetParam(NULL, "mss") != NULL, c->Expected == 0, "%d");
	    if (thGetParam(NULL, "mss"))
		success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");
	    }

	/*** An auth file the lexer cannot read is an error, not a way in.  No
	 *** entry could hold the NUL bytes it is given here.
	 ***/
	memset(contents, '\0', 8);
	if (!authFileSet(contents, 8)) return false;
	success &= EXPECT_EQL(mssAuthenticate(USERNAME, PASSWORD, 0), -1, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");

	/*** An auth file that is not there at all is an error, not a way in.
	 *** Reporting it logs, which is test 09's business rather than this
	 *** one's, so stdout is put away for the two calls.  The file has to
	 *** come back under the name the module was given, so it is written
	 *** rather than created afresh.
	 ***/
	close(auth_fd);
	if (!tmpFileDeInit(auth_path)) return false;
	if (!quietStart(&saved_stdout)) return false;
	rval = mssAuthenticate(USERNAME, PASSWORD, 0);
	bypass_rval = mssAuthenticate(USERNAME, PASSWORD, 1);
	if (!quietEnd(saved_stdout)) return false;
	success &= EXPECT_EQL(rval, -1, "%d");
	success &= EXPECT_EQL(bypass_rval, -1, "%d");
	success &= EXPECT_EQL(thGetParam(NULL, "mss"), NULL, "%p");
	auth_fd = open(auth_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (auth_fd < 0)
	    {
	    perror("doTest: could not put the auth file back");
	    return false;
	    }

    return success;
    }

long long test(char** tname)
    {
    long long ops;
    long long result;
    int i;

	*tname = "mtsession-02 Auth File Entries";

	/** The auth file is rewritten per case; its name stays the same, so
	 ** one initialization covers them all.
	 **/
	if (!tmpFileInit(auth_path, sizeof(auth_path))) return -1;
	auth_fd = open(auth_path, O_WRONLY | O_TRUNC, 0600);
	if (!authCred(cred, PASSWORD) || auth_fd < 0)
	    {
	    printFail("could not prepare the auth file");
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	mssInitialize("altpasswd", auth_path, "", 0, "test_mtsession");

	/** Each case checks its return value and whether a session appeared;
	 ** the cases that win one also check that it ends.  Five more checks
	 ** follow the loop.
	 **/
	ops = 5ll + 2ll * CASE_COUNT;
	for (i = 0; i < CASE_COUNT; i++)
	    if (!cases[i].Expected) ops++;

	result = loopTest(doTest) * ops;

	close(auth_fd);
	if (!tmpFileDeInit(auth_path)) return -1;

    return result;
    }

/** Scope cleanup. **/
#undef USERNAME
#undef PASSWORD
#undef CASE_COUNT
