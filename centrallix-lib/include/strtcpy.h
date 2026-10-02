#ifndef _STRTCPY_H
#define _STRTCPY_H

#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

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


int strtcpy(char* dst, const char* src, size_t dstlen);
int strtcat(char* dst, const char* src, size_t dstlen);
int strtcatf(char* dst, size_t dstlen, size_t* pos, const char* fmt, ...);
int strtcatf_va(char* dst, size_t dstlen, size_t* pos, const char* fmt, va_list ap);

#endif /* not defined _STRTCPY_H */
