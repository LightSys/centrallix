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

/*** Function for failing on error, assuming the error came from a library or
 *** system function call, so that the error buffer is set to a valid value.
 ***/
void print_err(int code, const char* function_name, const char* file_name, const int line_number)
    {
	/** Create a descriptive error message. **/
	char error_buf[BUFSIZ];
	snprintf(error_buf, sizeof(error_buf), "%s:%d: %s failed", file_name, line_number, function_name);
	
	/** Print it with as much info as we can reasonably find. **/
	if (errno != 0) perror(error_buf);
	else if (code != 0) fprintf(stderr, "%s (error code %d).\n", error_buf, code);
	else fprintf(stderr, "%s.\n", error_buf);
    
    return;
    }
