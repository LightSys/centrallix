#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "obj.h"
#include "htmlparse.h"
#include "stparse_ne.h"
#include "cxlib/cxsec.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	htmlparse.c,htmlparse.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 23, 1999   					*/
/* Description:	Provides HTML parsing facilities for converting to FX	*/
/*		and PCL style, and also possibly later for parsing HTML	*/
/*		documents to re-point embedded URL's that are converted	*/
/*		by the proxy agent... hmmm...				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htmlparse.c,v 1.6 2008/03/29 02:26:17 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/utility/htmlparse.c,v $

    $Log: htmlparse.c,v $
    Revision 1.6  2008/03/29 02:26:17  gbeeley
    - (change) Correcting various compile time warnings such as signed vs.
      unsigned char.

    Revision 1.5  2007/04/08 03:52:01  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.4  2006/11/16 20:15:54  gbeeley
    - (change) move away from emulation of NS4 properties in Moz; add a separate
      dom1html geom module for Moz.
    - (change) add wgtrRenderObject() to do the parse, verify, and render
      stages all together.
    - (bugfix) allow dropdown to auto-size to allow room for the text, in the
      same way as buttons and editboxes.

    Revision 1.3  2005/02/26 06:39:22  gbeeley
    * SECURITY FIX: possible heap corruption bug in htmlparse.c regarding
      illegal placement of % and & (and other chars) in the URL.

    Revision 1.2  2001/10/16 23:53:02  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.1.1.1  2001/08/13 18:01:17  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:18  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** hts_internal_ReadBuffer - read data from the input stream into the
 *** buffer for further processing.  This routine will rearrange the buffer
 *** and modify SetPtr and BufPtr as needed to make room for more data.
 ***/
int
hts_internal_ReadBuffer(pHtmlSession this)
    {
    int max_can_get = HTS_READ_SIZE;
    int n;
    char* read_ptr;
    char* ck_ptr;

    	/** Determine how much and where we can read. **/
	if (this->BufCnt <= HTS_BUF_SIZE - HTS_READ_SIZE)
	    {
	    read_ptr = this->Buffer + this->BufCnt;
	    }
	else 
	    {
	    if ((!this->SetPtr || this->SetPtr > this->Buffer) && this->BufPtr > this->Buffer)
	        {
		if (this->SetPtr && this->SetPtr < this->BufPtr)
		    ck_ptr = this->SetPtr;
		else
		    ck_ptr = this->BufPtr;
		n = this->BufCnt - (ck_ptr - this->Buffer);
		if (n) memmove(this->Buffer, ck_ptr, n);
		if (this->SetPtr) this->SetPtr -= (ck_ptr - this->Buffer);
		this->BufPtr -= (ck_ptr - this->Buffer);
		this->BufCnt -= (ck_ptr - this->Buffer);
		}
	    read_ptr = this->Buffer + this->BufCnt;
	    if (this->BufCnt > HTS_BUF_SIZE - HTS_READ_SIZE) max_can_get = HTS_BUF_SIZE - this->BufCnt;
	    if (max_can_get == 0) return HTS_CH_ERROR;
	    }

	/** Read data. **/
	n = this->ReadFn(this->ReadArg, read_ptr, max_can_get, 0,0);
	if (n <= 0) return HTS_CH_EOF;
	this->BufCnt += n;

    return n;
    }


/*** hts_internal_ReadCh - read a single character in, and return a pointer
 *** to the thing.
 ***/
int
hts_internal_ReadCh(pHtmlSession this)
    {
    int rval;

    	/** Chars in buf already? **/
	if (this->BufCnt > (this->BufPtr - this->Buffer)) return *(this->BufPtr++);

	/** Read more data. **/
	rval = hts_internal_ReadBuffer(this);
	if (rval < 0) return rval;

    return *(this->BufPtr++);
    }


/*** hts_internal_PeekCh - peek at the next character that's coming...
 ***/
int
hts_internal_PeekCh(pHtmlSession this)
    {
    int rval;

    	/** Chars in buf already? **/
	if (this->BufCnt > (this->BufPtr - this->Buffer)) return *(this->BufPtr);

	/** Read more data. **/
	rval = hts_internal_ReadBuffer(this);
	if (rval < 0) return rval;

    return *(this->BufPtr);
    }


/*** Read a 'converted' character -- that is, if in the &xxx; format, it will
 *** read from & through ; and convert it to a 'real' character.
 ***/
int
hts_internal_ReadConvCh(pHtmlSession this)
    {
    int ch;
    unsigned char buf[16];
    int n=0;

    	/** Chars in the 'put back' list? **/
	if (this->PutBackCnt) 
	    {
	    this->ConvBufPtr++;
	    return this->ConvBuf[this->ConvBufCnt - (this->PutBackCnt--)];
	    }

    	/** Read a character. **/
	ch = hts_internal_ReadCh(this);
	if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r') ch = HTS_CH_WHITESPACE;
	if (ch == '<') ch = HTS_CH_TAGSTART;
	if (ch == '>') ch = HTS_CH_TAGEND;

	/** Ok, it is &xxx; format.  Convert it. **/
	if (ch == '&')
	    {
	    /** Read from & to ; **/
	    while((ch = hts_internal_ReadCh(this)) != ';')
	        {
		if (ch == HTS_CH_EOF || ch == HTS_CH_ERROR) break;
	        buf[n++] = ch;
		}

	    /** Which one? **/
	    if (ch == ';')
	        {
		buf[n] = 0;
	        if (!strcmp((char*)buf,"nbsp")) ch = ' ';
	        if (!strcmp((char*)buf,"amp")) ch = '&';
	        if (!strcmp((char*)buf,"gt")) ch = '>';
	        if (!strcmp((char*)buf,"lt")) ch = '<';
		}
	    }

	/** Put the ch in the conv buffer... is there room? **/
	if (this->ConvBufCnt >= HTS_BUF_SIZE)
	    {
	    if (!this->ConvSetPtr)
	        {
		memmove(this->ConvBuf, this->ConvBuf + (HTS_BUF_SIZE - HTS_READ_SIZE), HTS_READ_SIZE);
		this->ConvBufCnt -= HTS_READ_SIZE;
		}
	    else if (this->ConvSetPtr > this->ConvBuf)
	        {
		memmove(this->ConvBuf, this->ConvSetPtr, HTS_BUF_SIZE - (this->ConvSetPtr - this->ConvBuf));
		this->ConvBufCnt -= (this->ConvSetPtr - this->ConvBuf);
		this->ConvSetPtr = this->ConvBuf;
		}
	    }

	/** Still no room? **/
	if (this->ConvBufCnt >= HTS_BUF_SIZE) ch = HTS_CH_ERROR;
	this->ConvBuf[this->ConvBufCnt++] = ch;
	this->ConvBufPtr = this->ConvBuf + (this->ConvBufCnt-this->PutBackCnt-1);

    return ch;
    }


/*** hts_internal_PutBackCh - put a character back on to the list of chars
 *** waiting to be read.  The system already knows what the char is, so it
 *** doesn't need to be a param.
 ***/
int
hts_internal_PutBackCh(pHtmlSession this)
    {
    this->PutBackCnt++;
    this->ConvBufPtr = this->ConvBuf + (this->ConvBufCnt-this->PutBackCnt-1);
    return 0;
    }


/*** htsOpenSession - attach to an open objOpen or fdOpen type of stream
 *** and prepare to read tokens from the thing.
 ***/
pHtmlSession 
htsOpenSession(int (*read_fn)(), void* read_arg, int flags)
    {
    pHtmlSession hts;

    	/** Allocate the session structure **/
	hts = (pHtmlSession)nmMalloc(sizeof(HtmlSession));
	if (!hts) return NULL;
	
	/** Fill in the structure parts **/
	memset(hts, 0, sizeof(HtmlSession));
	hts->Flags = flags;
	hts->ReadFn = read_fn;
	hts->ReadArg = read_arg;
	hts->Type = HTS_T_BEGIN;
	hts->BufPtr = hts->Buffer;
	hts->BufCnt = 0;
	hts->SetPtr = NULL;

    return hts;
    }


/*** htsCloseSession - close an open session.  Does NOT close the fd or obj
 *** stream itself.
 ***/
int 
htsCloseSession(pHtmlSession this)
    {

    	/** Free the structure... **/
	nmFree(this, sizeof(HtmlSession));

    return 0;
    }


/*** htsNextToken - retrieve the next token in the HTML file, whether it is
 *** a string, tag, or comment.
 ***/
int 
htsNextToken(pHtmlSession this)
    {
    int i = 0;
    int n;
    int ch,prevch;

    	/** Reset the set ptr to free some space up, if needed **/
	this->ConvSetPtr = NULL;

	/** Not already in-string?  Skip over spaces if so. **/
	if (this->Type != HTS_T_STRING)
	    {
	    /*while(((ch = hts_internal_ReadConvCh(this)) == ' ') || ch == HTS_CH_WHITESPACE);*/
	    while((ch = hts_internal_ReadConvCh(this)) == HTS_CH_WHITESPACE);
	    }
	else
	    {
	    ch = hts_internal_ReadConvCh(this);
	    }

    	/** String coming here, or is it a tag? **/
	if (ch == HTS_CH_EOF)
	    {
	    this->Type = HTS_T_EOF;
	    return this->Type;
	    }
	else if (ch == HTS_CH_ERROR)
	    {
	    this->Type = HTS_T_ERROR;
	    return this->Type;
	    }
	else if (ch == HTS_CH_TAGSTART)
	    {
	    /** Tag or comment? **/
	    if ((ch=hts_internal_ReadConvCh(this)) == '!')
	        {
		/** it's comment **/
		hts_internal_ReadConvCh(this);
		hts_internal_ReadConvCh(this);
		this->Type = HTS_T_COMMENT;
		}
	    else
	        {
		/** is a tag. **/
		hts_internal_PutBackCh(this);
		this->Type = HTS_T_TAG;
		}

	    /** Now parse the subparts **/
	    this->ConvSetPtr = this->ConvBufPtr;
	    prevch = HTS_CH_WHITESPACE;
	    n = 0;
	    while((ch = hts_internal_ReadConvCh(this)) != HTS_CH_TAGEND && ch != HTS_CH_EOF)
	        {
		if (ch == HTS_CH_ERROR) 
		    {
		    this->Type = HTS_T_ERROR;
		    break;
		    }
		else if (prevch == '-' && ch == '-' && this->Type == HTS_T_COMMENT)
		    {
		    *(this->ConvBufPtr) = '\0';
		    *(this->ConvBufPtr-1) = '\0';
		    }
		else if (prevch == HTS_CH_WHITESPACE && ch != HTS_CH_WHITESPACE)
		    {
		    if (n < 16) this->TagParts[n++] = this->ConvBufPtr - this->ConvSetPtr;
		    }
		else if (prevch == '=' && ch != HTS_CH_WHITESPACE)
		    {
		    if (n > 0) this->TagValues[n-1] = this->ConvBufPtr - this->ConvSetPtr;
		    }
		if (ch == HTS_CH_WHITESPACE || ch == '=') *(this->ConvBufPtr) = '\0';
		prevch = ch;
		}
	    *(this->ConvBufPtr) = '\0';
	    this->nTagParts = n;
	    }
	else
	    {
	    /** Grab the string, up to HTS_READ_SIZE bytes **/
	    this->ConvSetPtr = this->ConvBufPtr;
	    this->Type = HTS_T_STRING;
	    i = 0;
	    while((ch = hts_internal_ReadConvCh(this)) != HTS_CH_TAGSTART && ch != HTS_CH_EOF && i < HTS_READ_SIZE)
	        {
		if (ch == HTS_CH_ERROR)
		    {
		    this->Type = HTS_T_ERROR;
		    break;
		    }
		i++;
		}
	    if (ch == HTS_CH_TAGSTART || ch == HTS_CH_EOF) hts_internal_PutBackCh(this);
	    }

    return this->Type;
    }


/*** htsStringVal - return the current string value of a STRING token.  
 *** This routine automatically strips leading spaces when needed and
 *** converts the &xxx; characters to their proper representation.  A
 *** contiguous string in HTML may translate to multiple of these 
 *** tokens depending on the internal buffering, etc.
 ***/
int 
htsStringVal(pHtmlSession this, char** string_ptr, int* string_len)
    {
    char* ptr;
    *string_ptr = this->ConvSetPtr;
    *string_len = (this->ConvBufPtr - this->ConvSetPtr) + 1;
    for(ptr = this->ConvSetPtr; *ptr && ptr<=this->ConvBufPtr; ptr++) if (*ptr == HTS_CH_WHITESPACE) *ptr = ' ';
    return 0;
    }


/*** htsTagPart - return a pointer to a space-separated part of an HTML
 *** tag.
 ***/
char* 
htsTagPart(pHtmlSession this, int part_num)
    {
    if (part_num < 16 && this->TagParts[part_num] != -1)
        {
        return this->ConvSetPtr + this->TagParts[part_num];
	}
    else
        {
	mssError(1,"HTS","Could not access HTML tag parameter");
        return NULL;
	}
    }


/*** htsTagValue - return a pointer to the value of a tag, or NULL if no
 *** value.  This is the part after the '=' in the HTML tag.
 ***/
char* 
htsTagValue(pHtmlSession this, int part_num)
    {
    if (part_num < 16 && this->TagValues[part_num] != -1)
        {
        return this->ConvSetPtr + this->TagValues[part_num];
	}
    else
        {
	mssError(1,"HTS","Could not access HTML tag parameter");
        return NULL;
	}
    }


/*** htsParseURL - parse a URL into a structinf structure having parameters
 *** as needed.
 ***/
/*** nht_internal_ParseURL - parses the url passed to the http server into
 *** the pathname and the named parameters.
 ***/
pStruct
htsParseURL(char* url)
    {
    pStruct main_inf;
    pStruct attr_inf;
    char* ptr;
    char* dst;
    char* last_slash;
    int len;

        /** Allocate the main inf for the filename. **/
        main_inf = stAllocInf_ne();
        if (!main_inf) return NULL;

        /** Find the length of the undecoded pathname **/
        len = strlen(url);

        /** Alloc some space for the decoded pathname, and copy. **/
        ptr = (char*)nmSysMalloc(len*3+1);
        if (!ptr) return NULL;
        dst = ptr;
	last_slash = strrchr(url,'\0')-2;
	while(last_slash > url && *last_slash != '/') --last_slash;
        while(*url && (*url != '?' || url < last_slash)) 
	    {
	    /* grb - this is done twice - see obj_internal_DoPathSegment() */
	    /*if (*url == '?') in_params=1;
	    if (*url == '/') in_params=0;
	    if (in_params)
	        *(dst++) = *(url++);
	    else
	        *(dst++) = htsConvertChar(&url);*/
	    *(dst++) = *(url++);
	    }
        *dst=0;
        main_inf->StrAlloc = 1;
        main_inf->StrVal = ptr;

        /** Step through any parameters, and alloc inf structures for them. **/
        if (*url == '?') url++;
        while(*url && *url != '/')
            {
	    /** Empty param? **/
	    while (*url == '&') url++;

            /** Alloc a structure for this param. **/
            attr_inf = stAllocInf_ne();
            if (!attr_inf)
                {
                stFreeInf_ne(main_inf);
                return NULL;
                }

            /** Copy the param name. **/
            dst = attr_inf->Name;
            while (*url && *url != '&' && *url != '/' && *url != '=' && (dst-(attr_inf->Name)) < 31)
                {
                *(dst++) = htsConvertChar(&url);
                }
            *dst=0;
            if (*url == '=') url++;

	    /** Name must comply with symbol name standards **/
	    if (cxsecVerifySymbol(attr_inf->Name) < 0)
		{
		/** Skip to end of param **/
		while(*url && (*url != '&')) url++;

		stFreeInf_ne(attr_inf);
		continue;
		}

	    /** Ok, at least have a good attr name **/
	    attr_inf->StrVal = "";
	    attr_inf->StrAlloc = 0;
            stAddInf_ne(main_inf, attr_inf);

            /** Copy the param value, alloc'ing a string for it **/
            ptr = strpbrk(url,"&/");
            if (!ptr) len = strlen(url);
            else len = ptr-url;
            ptr = (char*)nmSysMalloc(len*3+1);
            if (!ptr)
                {
                stFreeInf_ne(main_inf);
                return NULL;
                }
            dst=ptr;
            while(*url && *url != '&' && *url != '/')
                {
                *(dst++) = htsConvertChar(&url);
                }
            *dst=0;
            attr_inf->StrAlloc = 1;
            attr_inf->StrVal = ptr;
            attr_inf->Type = ST_T_ATTRIB;
            if (*url == '&') url++;
            }

    return main_inf;
    }


/*** htsConvertChar - converts a %xx type entry to the correct
 *** representation of it, as well as '+' to ' '.
 ***/
char
htsConvertChar(char** ptr)
    {
    char x[3];
    char ch;

        /** + converts to a space. **/
        if (**ptr == '+')
            {
            (*ptr)++;
            ch = ' ';
            }
        else if (**ptr == '%')
            {
	    (*ptr)++;
            x[0] = (*ptr)[0];
            x[1] = (*ptr)[1];
            x[2] = 0;
	    if ((x[0] >= '0' && x[0] <= '9') || (x[0] >= 'a' && x[0] <= 'f') || (x[0] >= 'A' && x[0] <= 'F')) (*ptr)++;
	    if ((x[1] >= '0' && x[1] <= '9') || (x[1] >= 'a' && x[1] <= 'f') || (x[1] >= 'A' && x[1] <= 'F')) (*ptr)++;
            ch = strtol(x,NULL,16);
            }
        else
            {
            ch = **ptr;
            (*ptr)++;
            }

    return ch;
    }

