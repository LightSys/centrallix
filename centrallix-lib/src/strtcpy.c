#include <string.h>
#include "strtcpy.h"
#include "expect.h"

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
/* Module:	strtcpy.h, strtcpy.c                                    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	April 14th, 2006                                        */
/*									*/
/* Description:	Provides strtcpy(), a Truncating strcpy(), which	*/
/*		both respects the bounds of the destination and makes	*/
/*		sure the result is null-terminated.			*/
/************************************************************************/


/*** Truncating string concatenation.
 *** 
 *** Appends the source string to the destination string, truncating the data
 *** if needed to avoid overflowing the destination buffer.
 *** 
 *** @param dst A pointer to the destination buffer.
 *** @param src A pointer to the source buffer to read from.
 *** @param dstlen The total allocated memory of the destination buffer (not
 *** 	the length of available space), used to avoid writing off the end of
 *** 	the buffer.
 *** @returns number of bytes actually copied, including null terminator.  If
 *** 	truncated, returns `-(bytes copied)`.
 ***/
int
strtcat(char* dst, const char* src, size_t dstlen)
    {
    if (UNLIKELY((!dstlen))) 
	return 0;

    /** Find end of current string **/
    char* endptr = memchr(dst, '\0', dstlen);
    if (UNLIKELY((!endptr))) 
	return 0;
    if (UNLIKELY((endptr == dst+dstlen))) 
	return 0;

    /** Call strtcpy to copy the bytes and null-terminate it. **/
    return strtcpy(endptr, src, dstlen - (endptr - dst));
    }


/*** Truncating string copy.
 *** 
 *** Truncates written data if the source data is too long to fit into the
 *** allocated destination buffer.
 *** 
 *** @param dst A pointer to the destination buffer.
 *** @param src A pointer to the source buffer to read from.
 *** @param dstlen The length of the destination buffer, used to avoid writing
 *** 	off the end of the allocated memory.
 *** @returns The number of bytes actually copied (including null terminator).
 *** 	If the data is truncated, returns `-(bytes copied)`, which is the same
 *** 	as `-(dstlen)`.
 ***/
int
strtcpy(char* dst, const char* src, size_t dstlen)
    {
    size_t cnt = 0;
    if (UNLIKELY((!dstlen))) 
	return 0;

    while (LIKELY(cnt < dstlen) && LIKELY((dst[cnt] = src[cnt]) != '\0')) 
	cnt++;

    if (UNLIKELY(cnt == dstlen)) 
	{
	dst[cnt-1] = '\0';
	return -dstlen;
	}

    return cnt+1;
#if 00
    /** test suite says above is faster than the below **/
    while (--dstlen && ((*(dst++)) = (*(src++))));
    dst[0] = '\0';
    if (UNLIKELY((!dstlen && (*dst != '\0')))) 
            return -origlen;
    return origlen - dstlen;
#endif
    }
