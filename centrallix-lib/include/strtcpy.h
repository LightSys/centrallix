#ifndef _STRTCPY_H
#define _STRTCPY_H

#include <unistd.h>
#include <stdlib.h>

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2006 LightSys Technology Services, Inc.           */
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


int strtcpy(char* dst, const char* src, size_t dstlen);

#endif /* not defined _STRTCPY_H */
