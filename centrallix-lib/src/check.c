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

/*** Function for printing an error when code fails.
 *** 
 *** @param error_code The error code number returned by a failing C function (or -1 if not applicable).
 *** @param c_str The C statement/value that failed, usually a function call.
 *** @param file_name The name of the file in which error occurred.
 *** @param line_number The line number in the file at which the error occurred.
 ***/
void
print_err_internal(const int error_code, const char* c_str, const char* file_name, const int line_number)
    {
	/** Create a clear, concise, and descriptive error message. **/
	char error_buf[BUFSIZ];
	snprintf(error_buf, sizeof(error_buf), "%s:%d: %s", file_name, line_number, c_str);
	
	/** Print it with as much info as we can reasonably find. **/
	if (errno != 0) perror(error_buf);
	else if (error_code != 0) fprintf(stderr, "%s (error code %d).\n", error_buf, error_code);
	else fprintf(stderr, "%s.\n", error_buf);
    
    return;
    }
