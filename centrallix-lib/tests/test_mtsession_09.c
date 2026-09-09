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
/* Module:	test_mtsession_09.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Test where mssError() and mssErrorErrno() send a	*/
/* 		message when there is no session to keep it, or		*/
/* 		when every error is to be logged.			*/
/************************************************************************/

#include <errno.h>
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

/** The program name the log lines are expected to carry. **/
#define APPNAME		"test_mtsession"

/** Big enough for the handful of log lines this test provokes. **/
#define CAPTURE_SIZE	1024

static char auth_path[256];
static char capture_path[256];
static char captured[CAPTURE_SIZE];
static char expected[CAPTURE_SIZE];
static int saved_stdout = -1;

/*** The file stdout is captured into stays open for the whole test, emptied
 *** at the start of each capture rather than opened again.
 ***/
static int capture_fd = -1;

/*** Send everything written to stdout to a file of our own, until
 *** captureEnd() puts it back.
 ***
 *** @returns true if successful, false otherwise.
 ***/
static bool captureStart(void)
    {

	fflush(stdout);
	if (ftruncate(capture_fd, 0) || lseek(capture_fd, 0, SEEK_SET) < 0)
	    {
	    perror("captureStart: could not empty the capture file");
	    return false;
	    }
	saved_stdout = dup(STDOUT_FILENO);
	if (saved_stdout < 0)
	    {
	    perror("captureStart: could not save stdout");
	    return false;
	    }
	if (dup2(capture_fd, STDOUT_FILENO) < 0)
	    {
	    perror("captureStart: could not redirect stdout");
	    close(saved_stdout);
	    return false;
	    }

    return true;
    }

/*** Put stdout back where it was.
 ***
 *** @returns Everything written to stdout since captureStart().
 ***/
static char* captureEnd(void)
    {
    int length;

	fflush(stdout);
	captured[0] = '\0';
	if (dup2(saved_stdout, STDOUT_FILENO) < 0)
	    perror("captureEnd: could not restore stdout");
	close(saved_stdout);
	saved_stdout = -1;
	length = pread(capture_fd, captured, sizeof(captured) - 1, 0);
	if (length < 0)
	    {
	    perror("captureEnd: could not read the captured text back");
	    return captured;
	    }
	if (length == (int)sizeof(captured) - 1)
	    printFail("the captured text did not fit the buffer");
	captured[length] = '\0';

    return captured;
    }

static bool doTest(void)
    {
    bool success = true;

	/*** An error raised outside a session has no stack to go on, so the
	 *** stdout log method is where it ends up, conversions and errno text
	 *** and all.  One capture covers the three of them.
	 ***/
	mssInitialize("altpasswd", auth_path, "stdout", 0, APPNAME);
	if (!captureStart()) return false;
	mssError(1, "MOD", "no session here");
	mssError(1, "MOD", "user %s, attempt %d", "testuser", 3);
	errno = ENOENT;
	mssErrorErrno(1, "MOD", "could not open it");
	snprintf(expected, sizeof(expected),
		APPNAME": MOD: no session here\n"
		APPNAME": MOD: user testuser, attempt 3\n"
		APPNAME": MOD: could not open it (%s)\n", strerror(ENOENT));
	success &= EXPECT_STR_EQL(captureEnd(), expected);

	/*** With a session to hold the message, and without being told to log
	 *** everything, the log stays quiet.
	 ***/
	if (!EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d")) return false;
	if (!captureStart()) return false;
	mssError(1, "MOD", "in session");
	errno = ENOENT;
	mssErrorErrno(0, "MOD", "in session too");
	success &= EXPECT_STR_EQL(captureEnd(), "");
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");

	/*** Being told to log everything logs the messages that a session
	 *** would otherwise have kept to itself, and stacks them all the same.
	 ***/
	mssInitialize("altpasswd", auth_path, "stdout", 1, APPNAME);
	if (!EXPECT_EQL(check(mssAuthenticate(USERNAME, PASSWORD, 0)), 0, "%d")) return false;
	if (!captureStart()) return false;
	mssError(1, "MOD", "logged as well");
	errno = ENOENT;
	mssErrorErrno(0, "MOD", "logged too");
	snprintf(expected, sizeof(expected),
		APPNAME": MOD: logged as well\n"
		APPNAME": MOD: logged too (%s)\n", strerror(ENOENT));
	success &= EXPECT_STR_EQL(captureEnd(), expected);
	success &= EXPECT_EQL(((pMtSession)thGetParam(NULL, "mss"))->ErrList.nItems, 2, "%d");
	success &= EXPECT_EQL(check(mssEndSession(NULL)), 0, "%d");

	/** With no program name to log under, the lines say "error". **/
	mssInitialize("altpasswd", auth_path, "stdout", 0, "");
	if (!captureStart()) return false;
	mssError(1, "MOD", "nameless");
	success &= EXPECT_STR_EQL(captureEnd(), "error: MOD: nameless\n");

    return success;
    }

long long test(char** tname)
    {
    long long result;

	*tname = "mtsession-09 Error Logging";

	if (!tmpFileInit(auth_path, sizeof(auth_path))) return -1;
	if (!authFileWriteUser(auth_path, USERNAME, PASSWORD))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	if (!tmpFileInit(capture_path, sizeof(capture_path)))
	    {
	    tmpFileDeInit(auth_path);
	    return -1;
	    }
	capture_fd = open(capture_path, O_RDWR | O_TRUNC, 0600);
	if (capture_fd < 0)
	    {
	    printFail("could not open the file to capture stdout into");
	    tmpFileDeInit(capture_path);
	    tmpFileDeInit(auth_path);
	    return -1;
	    }

	result = loopTest(doTest) * 13ll;

	close(capture_fd);
	if (!tmpFileDeInit(capture_path)) result = -1;
	if (!tmpFileDeInit(auth_path)) result = -1;

    return result;
    }

/** Scope cleanup. **/
#undef USERNAME
#undef PASSWORD
#undef APPNAME
#undef CAPTURE_SIZE
