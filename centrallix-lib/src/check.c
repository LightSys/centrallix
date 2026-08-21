/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      check.c, check.h                                        */
/* Author:      Israel Fuller                                           */
/* Date:        October 13, 2025                                        */
/* Description: A utility to help with error checking on function       */
/*              return values, especially for library functions.        */
/************************************************************************/

#include <errno.h>
#include <stdio.h>

#include "check.h"
#include "strtcpy.h"

/*** Function for printing an error when code fails.
 *** 
 *** @param error_code The error code number returned by a failing C function (or -1 if not applicable).
 *** @param c_str The C statement/value that failed, usually a function call.
 *** @param file_name The name of the file in which error occurred.
 *** @param line_number The line number in the file at which the error occurred.
 ***/
void
printErrInternal(const int error_code, const char* c_str, const char* file_name, const int line_number)
    {
	/** Grab errno before any library call of ours can overwrite it. **/
	const int saved_errno = errno;
	size_t i = 0;
	char error_buf[BUFSIZ];

	/** Initialize buffer. **/
	error_buf[0] = '\0';

	/** Create a clear, concise, and descriptive error message. **/
	strtcatf(error_buf, sizeof(error_buf), &i, "%s:%d: %s", file_name, line_number, c_str);

	/** Fill it out with as much info as we can reasonably find. **/
	if (error_code != -1)
	    strtcatf(error_buf, sizeof(error_buf), &i, " (error code %d)", error_code);
	if (saved_errno != 0)
	    strtcatf(error_buf, sizeof(error_buf), &i, ": %s", strerror(saved_errno));

	/** Print the error message. **/
	if (i == 0)
	    /** Failed to make error message. Fallback to a more basic error. **/
	    fprintf(stderr,
		"%s:%d: %s. (Failed to build full error message.)\n",
		file_name, line_number, c_str
	    );
	else
	    fprintf(stderr, "%s.\n", error_buf);

    return;
    }
