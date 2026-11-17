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
#include "cxlib/cxsec.h"
#include "cxlib/hex.h"
#include "cxss/cxss.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
	if (hexEncode(hexkey, len + 1, hexkey, len/2+1, HEX_F_TRUNCATE) < 0)
	    return -1;

    return rval;
    }



/*** cxssShred() - Erase the given data so that it is no longer readable
 *** even in raw memory.  This is the same as calling memset_explicit(),
 *** except that this function works before C23, when memset_explicit()
 *** was added.
 *** 
 *** Also, using this function signifies the intent to scrub possibly
 *** sensitive data, which makes code more readable.
 *** 
 *** @param data A pointer to the data buffer to be erased.
 *** @param n_bytes The number of bytes allocated to the data buffer.
 *** 	Causes undefined behavior if incorrect.
 ***/
void
cxssShred(void* data, size_t n_bytes)
    {
	cxsecShred(data, n_bytes);

    return;
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
