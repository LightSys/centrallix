#ifndef _MIME_H
#define _MIME_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module:	mime.c,mime.h                                           */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	Way back in time                                        */
/*									*/
/* Description:	This module parses MIME-based messages into the msg	*/
/*		subparts.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: mime.h,v 1.1 2001/08/13 18:00:53 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/mime.h,v $

    $Log: mime.h,v $
    Revision 1.1  2001/08/13 18:00:53  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:20  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "xstring.h"
#include "xarray.h"

#define	MIME_TEXT		1
#define MIME_MULTIPART		2
#define MIME_APPLICATION	3
#define MIME_MESSAGE		4
#define MIME_IMAGE		5
#define MIME_AUDIO		6
#define MIME_VIDEO		7


/** information about a Received/to/cc/etc hdr **/
typedef struct _HD
    {
    XString	Buffer;
    XArray	NameOffsets;
    XArray	ValueOffsets;
    }
    MimeHdrData, *pMimeHdrData;


/** information structure for MIME msg **/
typedef struct _MM
    {
    XString	Buffer;
    int		ContentMainType;
    XArray	NameOffsets;
    XArray	ValueOffsets;
    XArray	SubParts;
    XArray	ReceivedHdrs;
    XArray	ToHdrs;
    XArray	CcHdrs;
    XArray	NewsgroupsHdrs;
    int		Flags;
    int		Status;
    int		MsgSeekStart;
    int		HdrSeekEnd;
    int		MsgSeekEnd;
    }
    MimeMsg, *pMimeMsg;

#endif
