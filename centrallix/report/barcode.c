#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "barcode.h"
#include "cxlib/mtsession.h"

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
/* Module: 	barcode.c,barcode.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 14, 1998					*/
/* Description:	Generates the appropriate sequences for postal barcodes	*/
/*		given the zip+9+2+ckdigit.  The output sequences are	*/
/*		returned as a sequence of ascii '0' and ascii '1',	*/
/*		where 0 is short bar and 1 is tall bar.  No grizzly	*/
/*		bars.							*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: barcode.c,v 1.2 2005/02/26 06:42:40 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/barcode.c,v $

    $Log: barcode.c,v $
    Revision 1.2  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.1.1.1  2001/08/13 18:01:12  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:13  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/** Various numeric sequences for postal bar code **/
char* postal_bar_seq[10] = 
    {
    "11000","00011","00101","00110","01001",
    "01010","01100","10001","10010","10100",
    };


/*** barEncodeNumeric - encode a source unencoded string of numbers from
 *** the set of digits (i.e., "302900444859" for "285 Lynnwood 30290-0444")
 *** into a binary sequence of the characters "0" and "1" of length 5n+2+5
 *** where n is length of the unencoded string.  If maxlen doesn't permit
 *** that length (with the \0 terminator), then this function fails.
 *** This function skips over '-' and ' ', but exits with error on any
 *** other non-numeric character encountered.  Length must be at least 5.
 *** This function auto calculates the check digit and appends it to the
 *** end of the encoded barcode.
 ***/
int 
barEncodeNumeric(int type, char* unencoded, int len, char* encoded, int maxlen)
    {
    int i=0;
    char* ptr;
    int ck=0;

	/** Only understand 'bar postal' right now **/
	if (type != BAR_T_POSTAL) 
	    {
	    mssError(1,"BAR","Invalid barcode type");
	    return -1;
	    }

	/** No bar code if length isn't at least 5. **/
	if (len < 5) 
	    {
	    mssError(1,"BAR","Barcode must have a numeric length of at least 5");
	    return -1;
	    }

	/** Verify length **/
	if (maxlen <= strlen(unencoded)*5+2+5) 
	    {
	    mssError(1,"BAR","Encoded barcode would exceed internal size limits");
	    return -1;
	    }

	/** Start with a '1' bar code for leading edge **/
	encoded[i++] = '1';

	/** Scan through the unencoded, doing conversion **/
	ptr = unencoded;
	while(*ptr && len)
	    {
	    if (*ptr == '-' || *ptr == ' ')
		{
		ptr++;
		continue;
		}
	    if (*ptr < '0' || *ptr > '9') 
	        {
		mssError(1,"BAR","Barcode must only contain numeric characters");
		return -1;
		}
	    ck += ((*ptr)-'0');
	    memcpy(encoded+i, postal_bar_seq[(*ptr)-'0'], 5);
	    i+=5;
	    ptr++;
	    len--;
	    }

	/** Add the check digit **/
	memcpy(encoded+i, postal_bar_seq[(10-(ck%10))%10], 5);
	i+=5;

	/** End with a '1' bar code for trailing edge **/
	encoded[i++] = '1';

	/** Null terminate **/
	encoded[i] = '\0';

    return i;
    }

