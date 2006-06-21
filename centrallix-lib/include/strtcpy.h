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

/**CVSDATA***************************************************************

    $Id: strtcpy.h,v 1.1 2006/06/21 21:22:44 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/strtcpy.h,v $

    $Log: strtcpy.h,v $
    Revision 1.1  2006/06/21 21:22:44  gbeeley
    - Preliminary versions of strtcpy() and qpfPrintf() calls, which can be
      used for better safety in handling string data.

 **END-CVSDATA***********************************************************/

int strtcpy(char* dst, const char* src, size_t dstlen);

#endif /* not defined _STRTCPY_H */
