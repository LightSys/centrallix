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
/* Module:      warn.c, warn.h                                          */
/* Author:      Israel Fuller                                           */
/* Date:        October 13, 2025                                        */
/* Description: A utility wrapper to print warnings when a function     */
/*              call misbehaves.  Not for printing errors.              */
/************************************************************************/

#include <errno.h>
#include <stdio.h>

#include "mtsession.h"
#include "strtcpy.h"
#include "warn.h"

#define ERR_BUF_SIZE 1024

/*** Function for printing an error when code fails.
 *** 
 *** @param error_code The error code number returned by a failing C function (or -1 if not applicable).
 *** @param c_str The C statement/value that failed, usually a function call.
 *** @param file_name The name of the file in which error occurred.
 *** @param line_number The line number in the file at which the error occurred.
 ***/
void
printWarningInternal(const int error_code, const char* c_str, const char* file_name, const int line_number)
    {
	/** Store errno before any library call of ours can overwrite it. **/
	const int saved_errno = errno;

	/** Collect as much extra info as we can get. **/
	char extra_info_buf[ERR_BUF_SIZE] = {'\0'};
	size_t i = 0;
	if (saved_errno != 0)
	    strtcatf(extra_info_buf, sizeof(extra_info_buf), &i, ": %s", strerror(saved_errno));
	if (error_code != -1)
	    strtcatf(extra_info_buf, sizeof(extra_info_buf), &i, " (error code %d)", error_code);

#ifndef CX_TESTING /* Skip warnings in tests to reduce noise. */
	/** Print the warning message. **/
	fprintf(stderr,
	    "%s:%d: Warning! %s%s.\n",
	    file_name, line_number, c_str, extra_info_buf
	);
#endif

    return;
    }
