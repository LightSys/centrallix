#ifndef _XSTRING_H
#define _XSTRING_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	xstring.c, xstring.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 17, 1998					*/
/* Description:	Extensible string support module.  Implements an auto-	*/
/*		realloc'ing string data structure.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xstring.h,v 1.2 2001/10/03 15:31:31 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xstring.h,v $

    $Log: xstring.h,v $
    Revision 1.2  2001/10/03 15:31:31  gbeeley
    Added xsPrintf and xsConcatPrintf functions to the xstring library.
    They currently support %s and %d with field width and precision.

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:03  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#define XS_BLK_SIZ	256

typedef struct _XS
    {
    char	InitBuf[XS_BLK_SIZ];
    char*	String;
    int		Length;
    int		AllocLen;
    }
    XString, *pXString;


/** XString methods **/
int xsInit(pXString this);
int xsDeInit(pXString this);
int xsCheckAlloc(pXString this, int addl_needed);
int xsConcatenate(pXString this, char* text, int len);
int xsCopy(pXString this, char* text, int len);
char* xsStringEnd(pXString this);
int xsPrintf(pXString this, char* fmt, ...);
int xsConcatPrintf(pXString this, char* fmt, ...);

#endif /* _XSTRING_H */

