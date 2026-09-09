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
/* Module:	test_mtsession_06.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test mssPrintError(), which writes the error stack	*/
/* 		of the current session out to a file.			*/
/************************************************************************/

#include <fcntl.h>
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

/** Every printed error stack starts with this line. **/
#define STACK_HEAD	"ERROR - Session By Username ["USERNAME"]\r\n"

/*** Each printed line is built in a buffer of this size, so a line that
 *** would be longer is cut to fit, terminator included.
 ***/
#define LINE_SIZE	200

/** Big enough for any error stack this test prints. **/
#define PRINT_SIZE	1024

static char auth_path[256];
static char print_path[256];
static char printed[PRINT_SIZE];

/*** The file printed to stays open for the whole test, so that printing costs
 *** a write and a read rather than a pair of opens.  It only grows, so each
 *** print is read back from where the last one ended.
 ***/
static pFile print_file;
static int read_fd = -1;
static int read_offset;

/*** Print the error stack of the current session and read back what that
 *** added to the file.
 ***
 *** @param rval Receives what mssPrintError() returned.
 *** @returns The printed text, or an empty string if nothing was printed.
 ***/
static char* printError(int* rval)
    {
    int length;

	printed[0] = '\0';
	*rval = mssPrintError(print_file);
	length = pread(read_fd, printed, sizeof(printed) - 1, read_offset);
	if (length < 0)
	    {
	    perror("printError: could not read the printed text back");
	    return printed;
	    }
	if (length == (int)sizeof(printed) - 1)
	    printFail("the printed text did not fit the buffer");
	printed[length] = '\0';
	read_offset += length;

    return printed;
    }

static bool doTest(void)
    {
    bool success = true;
    char long_message[LINE_SIZE * 2];
    int rval = 0;

	/** Outside a session there is no stack to print. **/
	success &= EXPECT_STR_EQL(printError(&rval), "");
	success &= EXPECT_EQL(rval, -1, "%d");

	success &= EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d");

	/** An empty stack prints as just its heading. **/
	success &= EXPECT_STR_EQL(printError(&rval), STACK_HEAD);
	success &= EXPECT_EQL(check(rval), 0, "%d");

	/** Messages print newest first, one line each. **/
	mssError(1, "MOD", "first");
	mssError(0, "MOD2", "second");
	success &= EXPECT_STR_EQL(printError(&rval),
		STACK_HEAD"--- MOD2: second\r\n--- MOD: first\r\n");
	success &= EXPECT_EQL(check(rval), 0, "%d");

	/** Printing leaves the stack as it was, so the same print repeats. **/
	success &= EXPECT_STR_EQL(printError(&rval),
		STACK_HEAD"--- MOD2: second\r\n--- MOD: first\r\n");

	/** A message too long for one printed line is cut to fit the line
	 ** buffer, which takes the line ending with it.
	 **/
	memset(long_message, 'L', sizeof(long_message) - 1);
	long_message[sizeof(long_message) - 1] = '\0';
	mssError(1, "MOD", "%s", long_message);
	success &= EXPECT_EQL((int)strlen(printError(&rval)),
		(int)strlen(STACK_HEAD) + LINE_SIZE - 1, "%d");
	success &= EXPECT_EQL(strncmp(printed + strlen(STACK_HEAD), "--- MOD: LLL", 12), 0, "%d");
	success &= EXPECT_EQL(check(rval), 0, "%d");

	/** The stack empties and prints as its heading again. **/
	success &= EXPECT_EQL(check(mssClearError()), 0, "%d");
	success &= EXPECT_STR_EQL(printError(&rval), STACK_HEAD);
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");

    return success;
    }

long long test(char** tname)
    {
    long long result;

	*tname = "mtsession-06 Printing The Error Stack";

	if (!tmpFileInit(auth_path, sizeof(auth_path))) return -1;
	if (!authFileWriteUser(auth_path, USERNAME, PASSWORD))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	if (!tmpFileInit(print_path, sizeof(print_path)))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	print_file = fdOpen(print_path, O_WRONLY | O_TRUNC, 0600);
	read_fd = open(print_path, O_RDONLY);
	if (!print_file || read_fd < 0)
	    {
	    printFail("could not open the file to print to");
	    tmpFileDeInit(print_path);
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	mssInitialize("altpasswd", auth_path, "", 0, "test_mtsession");

	result = loopTest(doTest) * 17ll;

	fdClose(print_file, 0);
	close(read_fd);
	if (!tmpFileDeInit(print_path)) result = -1;
	if (!tmpFileDeInit(auth_path)) result = -1;

    return result;
    }

/** Scope cleanup. **/
#undef USERNAME
#undef PASSWORD
#undef STACK_HEAD
#undef LINE_SIZE
#undef PRINT_SIZE
