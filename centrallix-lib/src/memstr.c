#include "memstr.h"

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2014 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:	memstr.h, memstr.c                                          */
/* Author:	Brady Steed (BLS)                                           */
/* Date:	July 8, 2014                                                */
/*									                                    */
/* Description:	Provides memstr(), a function that searches a buffer	*/
/*		for a string. Similar to strstr, but uses a size instead        */
/*		of null character for a delimiter.                              */
/************************************************************************/

/** branch prediction pseudo-macro - define if compiler doesn't support it **/
#ifndef __builtin_expect
#define __builtin_expect(e,c) (e)
#endif

/*** memstr() - fixed length string search
 *** 
 *** returns a pointer to the beginning of the substring
 *** found in the buffer.  Returns null if the substring
 *** is not found.
 ***/
char *
memstr(char* buffer, const char* substr, size_t buffer_len)
    {
    size_t substr_len, i;
    
    substr_len = strlen(substr);
    if(substr_len > buffer_len) return NULL;
    
    for(i = 0; i < buffer_len-substr_len; i++)
        {
        if(buffer[i] == substr[0] && strncmp(buffer + i, substr, substr_len) == 0)
            return buffer + i;
        }
    return NULL;
    }