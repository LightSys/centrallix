#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
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

    $Id: xstring.c,v 1.5 2002/08/06 16:39:25 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/xstring.c,v $

    $Log: xstring.c,v $
    Revision 1.5  2002/08/06 16:39:25  lkehresman
    Fixed a bug (that greg noticed) in the xsRTrim function that was
    inaccurately setting the string length.

    Revision 1.4  2002/08/06 16:00:29  lkehresman
    Added some xstring manipulation functions:
      xsRTrim - right trim whitespace
      xsLTrim - left trim whitespace
      xsTrim - xsRTrim && xsLTrim

    Revision 1.3  2001/10/03 15:48:09  gbeeley
    Added xsWrite() function to mimic fdWrite/objWrite for XStrings.

    Revision 1.2  2001/10/03 15:31:32  gbeeley
    Added xsPrintf and xsConcatPrintf functions to the xstring library.
    They currently support %s and %d with field width and precision.

    Revision 1.1.1.1  2001/08/13 18:04:23  gbeeley
    Centrallix Library initial import

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


int
xs_internal_Printf(pXString this, char* fmt, va_list vl)
    {
    char* cur_pos;
    char* ptr;
    int n;
    char* str;
    char nbuf[32];
    int i;
    int field_width= -999;
    int precision= -999;
    char* nptr;
    int found_field_width=0;

	/** Go through the format string one snippet at a time **/
	cur_pos = fmt;
	ptr = strchr(cur_pos, '%');
	if (ptr)
	    {
	    n = ptr - cur_pos;
	    xsConcatenate(this, cur_pos, n);
	    }
	while(ptr)
	    {
	    switch(ptr[1])
	        {
		case '\0':
		    xsCheckAlloc(this,1);
		    this->String[this->Length++] = '%';
		    cur_pos = ptr+1;
		    break;
		case '*':
		    if (field_width == -999 && !found_field_width)
			{
			field_width = va_arg(vl, int);
			found_field_width = 1;
			}
		    else
			precision = va_arg(vl, int);
		    ptr++;
		    continue; /* loop continue */
		case '.':
		    ptr++;
		    found_field_width=1;
		    continue;
		case '-':
		case '+':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    if (field_width == -999 && !found_field_width)
			{
			field_width = strtol(ptr+1,&nptr,10);
			found_field_width=1;
			}
		    else
			precision = strtol(ptr+1,&nptr,10);
		    ptr = nptr-1;
		    continue;
		case '%':
		    xsCheckAlloc(this,1);
		    this->String[this->Length++] = '%';
		    cur_pos = ptr+2;
		    break;
		case 's':
		    /** Get the string value **/
		    str = va_arg(vl, char*);
		    if (!str) str = "(null)";

		  do_as_string:	    /* from int handler, below */
		    n = strlen(str);

		    /** Need to pad beginning of string with spaces? **/
		    if (field_width > precision && precision >= 0)
			field_width = precision;
		    if (field_width > 0 && field_width > n)
			{
			xsCheckAlloc(this,field_width-n);
			memset(this->String+this->Length,' ',field_width-n);
			this->Length += (field_width-n);
			}

		    /** Add the string.  Need to make it less than given length? **/
		    if (n > precision && precision >= 0)
			n = precision;
		    xsConcatenate(this, str, n);

		    /** Add trailing blanks? **/
		    if (field_width < 0 && field_width != -999 && (-field_width) > n)
			{
			xsCheckAlloc(this,(-field_width)-n);
			memset(this->String+this->Length,' ',(-field_width)-n);
			this->Length += ((-field_width)-n);
			}
		    cur_pos = ptr+2;
		    found_field_width=0;
		    field_width = -999;
		    precision = -999;
		    break;
		case 'd':
		    i = va_arg(vl, int);
		    if (precision > 0)
			{
			if (precision >= 31) precision=30;
			sprintf(nbuf,"%.*d",precision,i);
			precision=-999;
			}
		    else
			{
			sprintf(nbuf,"%d",i);
			}
		    str = nbuf;
		    goto do_as_string; /* next section up */
		default:
		    cur_pos = ptr+2;
		    break;
		}
	    ptr = strchr(cur_pos, '%');
	    if (ptr)
		{
		n = ptr - cur_pos;
		xsConcatenate(this, cur_pos, n);
		}
	    }
	if (*cur_pos) xsConcatenate(this, cur_pos, -1);
	this->String[this->Length] = '\0';

    return 0;
    }


/*** xsConcatPrintf - prints to an XString using a printf-style format string,
 *** though the format string only allows %s and %d, no field widths or
 *** anything of that sort.
 ***/
int
xsConcatPrintf(pXString this, char* fmt, ...)
    {
    va_list vl;

	va_start(vl, fmt);
	xs_internal_Printf(this, fmt, vl);
	va_end(vl);

    return 0;
    }


/*** xsPrintf - prints to an XString using a printf-style format string,
 *** though the format string only allows %s and %d, no field widths or
 *** anything of that sort.
 ***/
int
xsPrintf(pXString this, char* fmt, ...)
    {
    va_list vl;

	va_start(vl, fmt);
	this->Length = 0;
	xs_internal_Printf(this, fmt, vl);
	va_end(vl);

    return 0;
    }


/*** xsWrite - writes data into an XString using the standard fdWrite/
 *** objWrite type of API.  This function can thus be used as a 
 *** write_fn for those functions that require it (such as the 
 *** expGenerateText() function).  WARNING: XStrings can hold
 *** more or less unlimited data.  Don't use this function if the
 *** calling API might write nearly unlimited data to it.
 ***/
int
xsWrite(pXString this, char* buf, int len, int offset, int flags)
    {

	/** If offset not specified, just a simple concat. **/
	if (!(flags & XS_U_SEEK))
	    {
	    xsConcatenate(this, buf, len);
	    }
	else
	    {
	    /** Check to see if end is overflowed. **/
	    if (offset+len > this->Length)
		{
		xsCheckAlloc(this, (offset+len) - this->Length);
		this->Length = offset+len;
		this->String[this->Length] = '\0';
		}
	    
	    /** Copy the data to the appropriate string position **/
	    memcpy(this->String + offset, buf, len);
	    }

    return len;
    }

/*** xsRTrim - Trims all the white space off of the right side
 *** of a given XString.  This is accomplished by moving the
 *** \0 character up to the desired location.
 ***/
int
xsRTrim(pXString this)
    {
    int i;

	for (i=this->Length-1; i >= 0 && 
		   (this->String[i] == '\r' ||
		    this->String[i] == '\n' ||
		    this->String[i] == '\t' ||
		    this->String[i] == ' '); i--);
	this->String[i+1] = '\0';
	this->Length = i+1;

    return 0;
    }

/*** xsLTrim - Trims all the white space off of the left side
 *** of a given XString.  This is accomplished by using memmove.
 ***/
int
xsLTrim(pXString this)
    {
    int i;

	for (i=0; i < this->Length && 
		   (this->String[i] == '\r' ||
		    this->String[i] == '\n' ||
		    this->String[i] == '\t' ||
		    this->String[i] == ' '); i++);
	memmove(this->String, this->String+i, this->Length-i);
	this->Length -= i;
	this->String[this->Length] = '\0';

    return 0;
    }

/*** xsTrim - Does both xsLTrim and xsRTrim on the given string
 ***/
int
xsTrim(pXString this)
    {

	xsLTrim(this);
	xsRTrim(this);

    return 0;
    }
