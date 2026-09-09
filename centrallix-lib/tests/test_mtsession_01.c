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
/* Module:	test_mtsession_01.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test mssGenCred(), which turns a password and salt	*/
/* 		into the credential that an auth file entry holds.	*/
/************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "check.h"

/** Tested module. **/
#include "mtsession.h"

/** Length of an MD5 credential, and the shortest buffer one fits in. **/
#define MD5_CRED_LEN	34
#define MD5_CRED_SIZE	35

/** Length of a DES credential, and the shortest buffer one fits in. **/
#define DES_CRED_LEN	13
#define DES_CRED_SIZE	14

/** A credential buffer, plus room past it for the guard bytes. **/
#define CRED_BUF_SIZE	80
#define GUARD_BYTE	'#'

/** Four salt bytes.  Each is expanded to its two hex digits, low digit
 ** first, so "salt" becomes "3716c647".
 **/
#define SALT		"salt"
#define SALT_HEX	"3716c647"

static char cred[CRED_BUF_SIZE];

/*** Generate a credential in a guarded buffer.  The whole buffer is filled
 *** with the guard byte first, so anything written past cred_maxlen is left
 *** for guardIntact() to find.
 ***/
static int genCred(char* salt, int salt_len, char* password, int cred_maxlen)
    {

	memset(cred, GUARD_BYTE, sizeof(cred));

    return mssGenCred(salt, salt_len, password, cred, cred_maxlen);
    }

/*** Check that nothing was written past the buffer the credential was
 *** allowed to use.
 ***/
static bool guardIntact(int cred_maxlen)
    {
    int i;

	for (i = cred_maxlen; i < CRED_BUF_SIZE; i++)
	    if (cred[i] != GUARD_BYTE) return false;

    return true;
    }

static bool doTest(void)
    {
    bool success = true;
    char other[CRED_BUF_SIZE];

	/** A buffer with room for an MD5 credential gets one, salted with the
	 ** hex expansion of the salt bytes.
	 **/
	success &= EXPECT_EQL(check(genCred(SALT, 4, "password", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_EQL(strncmp(cred, "$1$"SALT_HEX"$", 12), 0, "%d");
	success &= EXPECT_EQL((int)strlen(cred), MD5_CRED_LEN, "%d");
	success &= EXPECT_EQL(guardIntact(CRED_BUF_SIZE), true, "%d");

	/** The same inputs always produce the same credential. **/
	strcpy(other, cred);
	success &= EXPECT_EQL(check(genCred(SALT, 4, "password", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_STR_EQL(cred, other);

	/** A different password or a different salt does not. **/
	success &= EXPECT_EQL(check(genCred(SALT, 4, "password2", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_EQL(strcmp(cred, other) == 0, 0, "%d");
	success &= EXPECT_EQL(check(genCred("SALT", 4, "password", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_EQL(strcmp(cred, other) == 0, 0, "%d");

	/** Only salt_len bytes of the salt are used. **/
	success &= EXPECT_EQL(check(genCred(SALT, 2, "password", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_EQL(strncmp(cred, "$1$3716$", 8), 0, "%d");

	/** A NUL byte within salt_len is salt data like any other byte. **/
	success &= EXPECT_EQL(check(genCred("sa\0lt", 4, "password", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_EQL(strncmp(cred, "$1$371600c6$", 12), 0, "%d");
	success &= EXPECT_EQL(check(genCred("\0\0\0\0", 4, "password", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_EQL(strncmp(cred, "$1$00000000$", 12), 0, "%d");

	/** A salt longer than the optimum is cut to MSS_SALT_SIZE bytes
	 ** rather than overrunning the buffer it is expanded into.
	 **/
	success &= EXPECT_EQL(check(genCred(SALT, 4, "password", CRED_BUF_SIZE)), 0, "%d");
	strcpy(other, cred);
	success &= EXPECT_EQL(check(genCred(SALT"more", 8, "password", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_STR_EQL(cred, other);
	success &= EXPECT_EQL(check(genCred(SALT"more", 1000, "password", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_STR_EQL(cred, other);

	/** A salt length below one byte is refused outright. **/
	success &= EXPECT_EQL(genCred(SALT, 0, "password", CRED_BUF_SIZE), -1, "%d");
	success &= EXPECT_EQL(genCred(SALT, -1, "password", CRED_BUF_SIZE), -1, "%d");
	success &= EXPECT_EQL(guardIntact(0), true, "%d");

	/** An MD5 credential needs a buffer of MD5_CRED_SIZE bytes. **/
	success &= EXPECT_EQL(check(genCred(SALT, 4, "password", MD5_CRED_SIZE)), 0, "%d");
	success &= EXPECT_EQL((int)strlen(cred), MD5_CRED_LEN, "%d");
	success &= EXPECT_EQL(guardIntact(MD5_CRED_SIZE), true, "%d");

	/** One byte less falls back to a DES credential. **/
	success &= EXPECT_EQL(check(genCred(SALT, 4, "password", MD5_CRED_SIZE - 1)), 0, "%d");
	success &= EXPECT_EQL((int)strlen(cred), DES_CRED_LEN, "%d");
	success &= EXPECT_EQL(strncmp(cred, SALT_HEX, 2), 0, "%d");
	success &= EXPECT_EQL(guardIntact(MD5_CRED_SIZE - 1), true, "%d");

	/** A DES credential needs a buffer of DES_CRED_SIZE bytes. **/
	success &= EXPECT_EQL(check(genCred(SALT, 4, "password", DES_CRED_SIZE)), 0, "%d");
	success &= EXPECT_EQL((int)strlen(cred), DES_CRED_LEN, "%d");
	success &= EXPECT_EQL(guardIntact(DES_CRED_SIZE), true, "%d");

	/** A buffer too small for either kind of credential fails. **/
	success &= EXPECT_EQL(genCred(SALT, 4, "password", DES_CRED_SIZE - 1), -1, "%d");
	success &= EXPECT_EQL(genCred(SALT, 4, "password", 1), -1, "%d");
	success &= EXPECT_EQL(genCred(SALT, 4, "password", 0), -1, "%d");
	success &= EXPECT_EQL(genCred(SALT, 4, "password", -1), -1, "%d");
	success &= EXPECT_EQL(guardIntact(0), true, "%d");

	/** An empty password is still a password. **/
	success &= EXPECT_EQL(check(genCred(SALT, 4, "", CRED_BUF_SIZE)), 0, "%d");
	success &= EXPECT_EQL((int)strlen(cred), MD5_CRED_LEN, "%d");

    return success;
    }

long long test(char** tname)
    {
	*tname = "mtsession-01 Credential Generation";

	/*** The credential shapes below come from crypt(), which need not
	 *** offer either of them.  Check once, rather than reporting a failure
	 *** for every assertion that rests on it.
	 ***/
	if (genCred(SALT, 4, "password", CRED_BUF_SIZE)
		|| strncmp(cred, "$1$", 3) || (int)strlen(cred) != MD5_CRED_LEN)
	    {
	    printFail("crypt() here does not produce $1$ MD5 credentials");
	    return -1;
	    }
	if (genCred(SALT, 4, "password", DES_CRED_SIZE)
		|| (int)strlen(cred) != DES_CRED_LEN)
	    {
	    printFail("crypt() here does not produce DES credentials");
	    return -1;
	    }

    return loopTest(doTest) * 41ll;
    }

/** Scope cleanup. **/
#undef MD5_CRED_LEN
#undef MD5_CRED_SIZE
#undef DES_CRED_LEN
#undef DES_CRED_SIZE
#undef CRED_BUF_SIZE
#undef GUARD_BYTE
#undef SALT
#undef SALT_HEX
