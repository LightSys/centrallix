#include <stdarg.h>
#include <stdio.h>
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
/* Description:	Provides truncating string functions, which respect the	*/
/*		bounds of the destination and ensure null-termination.	*/
/************************************************************************/


/*** strtcat() - truncating string concatenation
 ***
 *** Appends to dst, being sure to not overflow the given dstlen size.
 *** Returns number of bytes actually copied, including null terminator.
 *** If truncated, returns -(bytes copied).
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


/*** strtcpy() - truncating string copy
 ***
 *** returns number of bytes actually copied (including null terminator)
 *** if truncated, returns -(bytes copied), which is the same as -(dstlen).
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


/*** strtcatf_va() - same as strtcatf(), but takes a va_list instead of
 *** a variable argument list.
 ***/
int
strtcatf_va(char* dst, size_t dstlen, size_t* pos, const char* fmt, va_list ap)
    {
    size_t start = *pos;
    int ret;

    /** No room for even one character. **/
    if (UNLIKELY((start + 1 >= dstlen))) 
	return 0;

    ret = vsnprintf(dst + start, dstlen - start, fmt, ap);

    /** vsnprintf() failed, so discard whatever it left behind. **/
    if (UNLIKELY((ret < 0))) 
	{
	dst[start] = '\0';
	return 0;
	}

    /** Output overran dst, so it was truncated and dst is now full. **/
    if (UNLIKELY((start + (size_t)ret >= dstlen))) 
	{
	*pos = dstlen - 1;
	return -(int)(dstlen - start);
	}

    *pos = start + (size_t)ret;
    return ret + 1;
    }


/*** strtcatf() - truncating formatted string concatenation
 ***
 *** Appends a printf-style message to dst, being sure to not overflow the
 *** given dstlen size.  *pos is the offset of dst's terminating null, and
 *** advances past the appended text, so chained calls need no checks in
 *** between.  A full dst, or a *pos outside it, appends nothing.
 *** Returns number of bytes actually appended, including null terminator.
 *** If truncated, returns -(bytes appended).
 ***/
int
strtcatf(char* dst, size_t dstlen, size_t* pos, const char* fmt, ...)
    {
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = strtcatf_va(dst, dstlen, pos, fmt, ap);
    va_end(ap);

    return ret;
    }
