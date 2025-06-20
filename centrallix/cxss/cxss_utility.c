#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxss/cxss.h"

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
/* Module:	cxss (Centrallix Security Subsystem)                    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	February 22, 2007                                       */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.			*/
/************************************************************************/


/*** cxss_i_Hexify - convert binary data into a hex string.  Supports doing an
 *** in-place conversion, i.e. bindata and hexdata referring to the same
 *** buffer starting address.
 ***
 *** This function fills as much of the buffer hexdata as is possible given
 *** the length of the bindata input.  If hexdata is too small, on the other
 *** hand, the conversion is truncated.  'hexdatalen' refers to the number of
 *** hex characters to convert; this function adds an additional nul
 *** terminator beyond those 'hexdatalen' characters.
 ***/
int
cxss_i_Hexify(unsigned char* bindata, size_t bindatalen, char* hexdata, size_t hexdatalen)
    {
    int i;
    char hex_chars[] = "0123456789abcdef";

	/** Ensure we have enough data, truncate if not. **/
	if (hexdatalen > bindatalen*2)
	    hexdatalen = bindatalen*2;

	/** Convert it, starting at the end **/
	for(i=hexdatalen-1; i>=0; i--)
	    {
	    hexdata[i] = hex_chars[(i&1)?(bindata[i/2] & 0x0f):((bindata[i/2]>>4) & 0x0f)];
	    }
	hexdata[hexdatalen] = '\0';

    return hexdatalen;
    }


/*** cxssHexify - this is cxss_i_Hexify with a more traditional interface.  The above
 *** function is different because it is optimized for generating a hex string of a
 *** given size, even if an odd length, rather than for converting all of a binary
 *** string to a hex string.
 ***/
int
cxssHexify(unsigned char* bindata, size_t bindatalen, char* hexdata, size_t hexdatabuflen)
    {

	/** Ensure buffer is large enough **/
	if (hexdatabuflen < bindatalen*2 + 1)
	    return -1;
	
    return cxss_i_Hexify(bindata, bindatalen, hexdata, bindatalen*2);
    }


/*** cxssGenerateKey - create a pseudorandom key of the specified size
 *** using cryptographically secure methods.
 ***/
int
cxssGenerateKey(unsigned char* key, size_t n_bytes)
    {
    int rval;

	/** Get bytes from the entropy pool **/
	rval = cxss_internal_GetBytes(key, n_bytes);

    return rval;
    }


/*** cxssGenerateHexKey - generate a random key in hexadecimal form.
 *** 'len' is the number of characters to generate, and this function
 *** will add an additional nul terminator past the 'len' characters.
 ***/
int
cxssGenerateHexKey(char* hexkey, size_t len)
    {
    int rval;
    
	/** Get entropy bytes first **/
	rval = cxssGenerateKey((unsigned char*)hexkey, len/2+1);
	if (rval < 0)
	    return rval;

	/** Convert to hex **/
	if (cxss_i_Hexify((unsigned char*)hexkey, len/2+1, hexkey, len) < 0)
	    return -1;

    return rval;
    }


/*** cxssShred - erase the given data so that it is no longer readable
 *** even in raw memory.  At this point, basically the same as memset().
 *** But using this function signifies the intent and makes the code
 *** more readable.
 ***/
int
cxssShred(unsigned char* data, size_t n_bytes)
    {
    memset(data, '\0', n_bytes);
    return 0;
    }



/*** cxssAddEntropy - stir some entropy into the entropy pool, and
 *** increase the estimate of available entropy by the given amount.
 ***/
int
cxssAddEntropy(unsigned char* data, size_t n_bytes, int entropy_bits_estimate)
    {
    int rval;

	/** Add it to the entropy pool **/
	rval = cxss_internal_AddToPool(data, n_bytes, entropy_bits_estimate);
	
    return rval;
    }

