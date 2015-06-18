/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2015 LightSys Technology Services, Inc.		*/
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
/* Module: 	libmime							*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	August 12, 2002						*/
/* Description:	Provides MIME parsing facilities used mainly in the	*/
/*		MIME object system driver (objdrv_mime.c)		*/
/************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "obj.h"
#include "mime.h"

int
libmime_EncodeQP()
    {
    return 0;
    }

int
libmime_DecodeQP()
    {
    return 0;
    }


int
libmime_EncodeBase64(unsigned char* dst, unsigned char* src, int maxdst)
    {
    static unsigned char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    /** Step through src 3 bytes at a time, generating 4 dst bytes for each 3 src **/
    while(*src)
	{
	/** First 6 bits of source[0] --> first byte dst. **/
	if (maxdst < 5)
	    {
	    mssError(1,"MIME","Could not encode MIME data field - internal resources exceeded");
	    return -1;
	    }
	dst[0] = b64[src[0]>>2];

	/** Second dst byte from last 2 bits of src[0] and first 4 of src[1] **/
	if (src[1] == '\0')
	    {
	    dst[1] = b64[(src[0]&0x03)<<4];
	    dst[2] = '=';
	    dst[3] = '=';
	    dst += 4;
	    break;
	    }
	dst[1] = b64[((src[0]&0x03)<<4) | (src[1]>>4)];

	/** Third dst byte from second 4 bits of src[1] and first 2 of src[2] **/
	if (src[2] == '\0')
	    {
	    dst[2] = b64[(src[1]&0x0F)<<2];
	    dst[3] = '=';
	    dst += 4;
	    break;
	    }
	dst[2] = b64[((src[1]&0x0F)<<2) | (src[2]>>6)];

	/** Last dst byte from last 6 bits of src[2] **/
	dst[3] = b64[(src[2]&0x3F)];

	/** Increment ctrs **/
	maxdst -= 4;
	dst += 4;
	src += 3;
	}

    /** Null-terminate the thing **/
    *dst = '\0';

    return 0;
    }

int
libmime_DecodeBase64(char* dst, char* src, int maxdst)
    {
    static char b64[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* ptr;
    char* origdst;
    int ix;

    /** Step through src 4 bytes at a time. **/
    origdst = dst;
    while(src[0] && src[1] && src[2] && src[3])
	{
	/** First 6 bits. **/
	if (maxdst < 4) 
	    {
	    mssError(1,"MIME","Could not decode MIME data field - internal resources exceeded");
	    return -1;
	    }
	ptr = strchr(b64,src[0]);
	if (!ptr) 
	    {
	    mssError(1,"MIME","Illegal character in MIME encoded data field");
	    return -1;
	    }
	ix = ptr-b64;
	dst[0] = ix<<2;

	/** Second six bits are split between dst[0] and dst[1] **/
	ptr = strchr(b64,src[1]);
	if (!ptr)
	    {
	    mssError(1,"MIME","Illegal character in MIME encoded data field");
	    return -1;
	    }
	ix = ptr-b64;
	dst[0] |= ix>>4;
	dst[1] = (ix<<4)&0xF0;

	/** Third six bits are nonmandatory and split between dst[1] and [2] **/
	if (src[2] == '=' && src[3] == '=')
	    {
	    maxdst -= 1;
	    dst += 1;
	    src += 4;
	    break;
	    }
	ptr = strchr(b64,src[2]);
	if (!ptr)
	    {
	    mssError(1,"MIME","Illegal character in MIME encoded data field");
	    return -1;
	    }
	ix = ptr-b64;
	dst[1] |= ix>>2;
	dst[2] = (ix<<6)&0xC0;

	/** Fourth six bits are nonmandatory and a part of dst[2]. **/
	if (src[3] == '=')
	    {
	    maxdst -= 2;
	    dst += 2;
	    src += 4;
	    break;
	    }
	ptr = strchr(b64,src[3]);
	if (!ptr)
	    {
	    mssError(1,"MIME","Illegal character in MIME encoded data field");
	    return -1;
	    }
	ix = ptr-b64;
	dst[2] |= ix;
	maxdst -= 3;
	src += 4;
	dst += 3;
	}

    dst[0] = '\0';

    return (dst - origdst);
    }
