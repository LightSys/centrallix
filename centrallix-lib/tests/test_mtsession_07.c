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
/* Module:	test_mtsession_07.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test session parameters: strings kept by the		*/
/* 		session, opaque pointers kept for the caller, and	*/
/* 		reading them back.					*/
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

/** Tested module. **/
#include "mtsession.h"

#define USERNAME	"testuser"
#define PASSWORD	"testpassword"

/*** A value of this length still fits in the parameter's own buffer; one
 *** byte more has to be allocated.  Both read back the same way.
 ***/
#define INLINE_LEN	63

/** Long enough to be stored away from the parameter. **/
#define LONG_LEN	200

static char auth_path[256];

/*** Fill a buffer with a NUL terminated run of one character.
 ***/
static char* fill(char* buf, char ch, int length)
    {

	memset(buf, ch, length);
	buf[length] = '\0';

    return buf;
    }

static bool doTest(void)
    {
    bool success = true;
    char inline_value[INLINE_LEN + 1];
    char long_value[LONG_LEN + 1];
    char alloc_value[INLINE_LEN + 2];
    char long_name[MSS_PARAMNAME_SIZE * 2];
    char fitting_name[MSS_PARAMNAME_SIZE];
    char over_name[MSS_PARAMNAME_SIZE + 1];
    char borrowed[16];
    int marker;

	fill(inline_value, 'i', INLINE_LEN);
	fill(alloc_value, 'a', INLINE_LEN + 1);
	fill(long_value, 'v', LONG_LEN);
	fill(long_name, 'n', MSS_PARAMNAME_SIZE * 2 - 1);
	strcpy(borrowed, "borrowed");

	/** Outside a session there are no parameters to set or read. **/
	success &= EXPECT_EQL(mssSetParam("param", "value"), -1, "%d");
	success &= EXPECT_EQL(mssSetParamPtr("param", &marker), -1, "%d");
	success &= EXPECT_EQL(mssGetParam("param"), NULL, "%p");

	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");

	/** A parameter that was never set reads back as nothing. **/
	success &= EXPECT_EQL(mssGetParam("param"), NULL, "%p");

	/** A string parameter reads back as an equal string, kept in the
	 ** session rather than in the caller's buffer.
	 **/
	success &= EXPECT_EQL(check(mssSetParam("param", "value")), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), "value");
	success &= EXPECT_EQL(mssGetParam("param") == (void*)"value", 0, "%d");

	/** Setting it again replaces the value, whatever the lengths. **/
	success &= EXPECT_EQL(check(mssSetParam("param", inline_value)), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), inline_value);
	success &= EXPECT_EQL(check(mssSetParam("param", long_value)), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), long_value);
	success &= EXPECT_EQL(check(mssSetParam("param", alloc_value)), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), alloc_value);
	success &= EXPECT_EQL(check(mssSetParam("param", "value")), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), "value");
	success &= EXPECT_EQL(check(mssSetParam("param", "")), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), "");

	/** Parameters are separate from one another. **/
	success &= EXPECT_EQL(check(mssSetParam("other", long_value)), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), "");
	success &= EXPECT_STR_EQL(mssGetParam("other"), long_value);

	/** A pointer parameter reads back as that same pointer, and NULL is
	 ** a value like any other.
	 **/
	success &= EXPECT_EQL(check(mssSetParamPtr("pointer", &marker)), 0, "%d");
	success &= EXPECT_EQL(mssGetParam("pointer"), (void*)&marker, "%p");
	success &= EXPECT_EQL(check(mssSetParamPtr("pointer", NULL)), 0, "%d");
	success &= EXPECT_EQL(mssGetParam("pointer"), NULL, "%p");

	/** Writing a string over a pointer parameter leaves the memory the
	 ** pointer referred to alone; it was never the session's to release.
	 **/
	success &= EXPECT_EQL(check(mssSetParamPtr("pointer", borrowed)), 0, "%d");
	success &= EXPECT_EQL(check(mssSetParam("pointer", long_value)), 0, "%d");
	success &= EXPECT_STR_EQL(borrowed, "borrowed");
	success &= EXPECT_STR_EQL(mssGetParam("pointer"), long_value);

	/** Writing a pointer over a string parameter works as well. **/
	success &= EXPECT_EQL(check(mssSetParamPtr("pointer", borrowed)), 0, "%d");
	success &= EXPECT_EQL(mssGetParam("pointer"), (void*)borrowed, "%p");

	/*** A name too long for the field it is kept in is refused, rather
	 *** than stored under a shortened name that would answer for every
	 *** other name sharing its start.
	 ***/
	success &= EXPECT_EQL(mssSetParam(long_name, "value"), -1, "%d");
	success &= EXPECT_EQL(mssSetParamPtr(long_name, borrowed), -1, "%d");
	success &= EXPECT_EQL(mssGetParam(long_name), NULL, "%p");

	/*** A name of the greatest length that fits is still a name, and the
	 *** refused one does not answer to it.
	 ***/
	strtcpy(fitting_name, long_name, sizeof(fitting_name));
	success &= EXPECT_EQL(check(mssSetParam(fitting_name, "value")), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam(fitting_name), "value");
	success &= EXPECT_EQL(mssGetParam(long_name), NULL, "%p");

	/** One character more than fits is one too many. **/
	strtcpy(over_name, long_name, sizeof(over_name));
	success &= EXPECT_EQL(mssSetParam(over_name, "value"), -1, "%d");
	success &= EXPECT_EQL(mssGetParam(over_name), NULL, "%p");

	/** There is no value to store for a NULL one, and no name either. **/
	success &= EXPECT_EQL(mssSetParam("param", NULL), -1, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), "");
	success &= EXPECT_EQL(mssSetParam(NULL, "value"), -1, "%d");
	success &= EXPECT_EQL(mssSetParamPtr(NULL, borrowed), -1, "%d");
	success &= EXPECT_EQL(mssGetParam(NULL), NULL, "%p");

	/*** The value given may be the one the parameter is already holding,
	 *** whether that is kept in the parameter or away from it.
	 ***/
	success &= EXPECT_EQL(check(mssSetParam("param", long_value)), 0, "%d");
	success &= EXPECT_EQL(check(mssSetParam("param", mssGetParam("param"))), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), long_value);
	success &= EXPECT_EQL(check(mssSetParam("param", "value")), 0, "%d");
	success &= EXPECT_EQL(check(mssSetParam("param", mssGetParam("param"))), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), "value");

	/*** Handing a parameter the value it is already holding as a pointer
	 *** changes nothing, and leaves the storage the session's own.
	 ***/
	success &= EXPECT_EQL(check(mssSetParam("param", long_value)), 0, "%d");
	success &= EXPECT_EQL(check(mssSetParamPtr("param", mssGetParam("param"))), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), long_value);

	/** It may even point part of the way into the value being replaced. **/
	success &= EXPECT_EQL(check(mssSetParam("param", "prefix body")), 0, "%d");
	success &= EXPECT_EQL(check(mssSetParam("param", (char*)mssGetParam("param") + 1)), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam("param"), "refix body");

	/** An empty name is a name too. **/
	success &= EXPECT_EQL(check(mssSetParam("", "value")), 0, "%d");
	success &= EXPECT_STR_EQL(mssGetParam(""), "value");

	/** Parameters belong to the session, so a new one starts with none. **/
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");
	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");
	success &= EXPECT_EQL(mssGetParam("param"), NULL, "%p");
	success &= EXPECT_EQL(mssGetParam("other"), NULL, "%p");
	success &= EXPECT_EQL(mssGetParam("pointer"), NULL, "%p");

	/** Parameters left behind are released with the session. **/
	success &= EXPECT_EQL(check(mssSetParam("param", long_value)), 0, "%d");
	success &= EXPECT_EQL(check(mssSetParam("other", "value")), 0, "%d");
	success &= EXPECT_EQL(check(mssSetParamPtr("pointer", borrowed)), 0, "%d");
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");
	success &= EXPECT_STR_EQL(borrowed, "borrowed");

    return success;
    }

long long test(char** tname)
    {
    long long result;

	*tname = "mtsession-07 Session Parameters";

	if (!tmpFileInit(auth_path, sizeof(auth_path))) return -1;
	if (!authFileWriteUser(auth_path, USERNAME, PASSWORD))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	mssInitialize("altpasswd", auth_path, "", 0, "test_mtsession");

	result = loopTest(doTest) * 68ll;

	if (!tmpFileDeInit(auth_path)) return -1;

    return result;
    }

/** Scope cleanup. **/
#undef USERNAME
#undef PASSWORD
#undef INLINE_LEN
#undef LONG_LEN
