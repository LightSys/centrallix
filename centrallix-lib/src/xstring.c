#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "xstring.h"
#include "newmalloc.h"

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

    $Id: xstring.c,v 1.1 2001/08/13 18:04:23 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/xstring.c,v $

    $Log: xstring.c,v $
    Revision 1.1  2001/08/13 18:04:23  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/07/03 01:02:57  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


/*** xsInit - initialize an existing XString structure.  The structure
 *** MUST be allocated first.
 ***/
int 
xsInit(pXString this)
    {
    	
	/** Initialize the various fields. **/
	this->String = this->InitBuf;
	this->String[0] = '\0';
	this->AllocLen = XS_BLK_SIZ;
	this->Length = 0;

    return 0;
    }


/*** xsDeInit - deinitialize the XString structure, freeing any added
 *** memory occupied by the string data.
 ***/
int 
xsDeInit(pXString this)
    {

    	/** If AllocLen > XS_BLK_SIZ, we allocated the memory. **/
	if (this->AllocLen > XS_BLK_SIZ) nmSysFree(this->String);
	this->AllocLen = 256;
	this->String = this->InitBuf;

    return 0;
    }


/*** xsCheckAlloc - optionally add more memory to the data if needed,
 *** given the currently occupied data area and the additional data
 *** required.
 ***/
int 
xsCheckAlloc(pXString this, int addl_needed)
    {
    int	new_cnt = 0;
    char* ptr;

    	/** See if more memory is needed. **/
	if (this->AllocLen < this->Length + addl_needed + 1)
	    {
	    new_cnt = (this->Length + addl_needed + XS_BLK_SIZ) & ~(XS_BLK_SIZ-1);

	    /** If only XS_BLK_SIZ alloc'd, we're using internal buffer. **/
	    if (this->AllocLen == XS_BLK_SIZ)
	        {
		ptr = (char*)nmSysMalloc(new_cnt);
		if (!ptr) return -1;
		this->String = ptr;
		memcpy(this->String,this->InitBuf,XS_BLK_SIZ);
		this->AllocLen = new_cnt;
		}
	    else
	        {
		ptr = (char*)nmSysRealloc(this->String, new_cnt);
		if (!ptr) return -1;
		this->String = ptr;
		this->AllocLen = new_cnt;
		}
	    }

    return 0;
    }


/*** xsConcatenate - adds text data to the end of the existing string, and
 *** allocs more memory as needed.  If 'len' is -1, then the length is 
 *** calculated using strlen(), otherwise the given length is enforced.
 ***/
int 
xsConcatenate(pXString this, char* text, int len)
    {

    	/** Determine length. **/
	if (len == -1) len = strlen(text);

    	/** Check memory **/
	if (xsCheckAlloc(this,len) < 0) return -1;

	/** Copy to end of string. **/
	memcpy(this->String + this->Length, text, len);
	this->Length += len;
	this->String[this->Length] = '\0';

    return 0;
    }


/*** xsCopy - copies text data to the given string, overwriting anything
 *** that is already there.  As before, 'len' can be -1.  Semantics of 'len'
 *** are same as that of xsConcatenate.
 ***/
int 
xsCopy(pXString this, char* text, int len)
    {

	/** Reset length to 0 and concatenate. **/
	this->Length = 0;
	if (xsConcatenate(this,text,len) < 0) return -1;

    return 0;
    }


/*** xsStringEnd - returns a pointer to the end of the string so that
 *** routines like 'sprintf' can be used (assuming that the caller made
 *** sure there was enough room with xsCheckAlloc()...)
 ***/
char* 
xsStringEnd(pXString this)
    {
    return this->String + this->Length;
    }


