#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include "cxsec.h"
#include <stdlib.h>
#include <stdio.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2003 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	cxsec.h                 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 31, 2003    					*/
/* Description:	This module implements some security hardening features	*/
/*		that are optional for application functioning.		*/
/************************************************************************/


/** set the key mask so that there is one byte of 'zero' **/
unsigned long cxsec_key = 0;
#define CXSEC_KEY_MASK	(0xFFFF00FF)

void 
cxsecCheckCanary(unsigned long* ck, unsigned long c, char* file, int line)
    {
    if (!(ck[0] == c && ck[1] == c))
	{
	printf("security violation: %s[%d]: canary check failed\n", file, line);
	abort();
	}
    return;
    }


void
cxsecInitialize()
    {
    /** Only init if we haven't done it already **/
    while (!cxsec_key) cxsec_key = rand() & CXSEC_KEY_MASK;
    return;
    }


/** These routines implement the data structure verification.  Right now
 ** we just use a fairly weak XOR checksum because things like MD5 are
 ** simply impractical.
 **/
void
cxsecInitDS(unsigned long* start, unsigned long* end)
    {
    unsigned long* ptr;
    unsigned long cksum = 0;
    start[0] = rand() & CXSEC_KEY_MASK;
    end[-1] = 0;
    end[0] = start[0] ^ cxsec_key;
    for(ptr=start; ptr<end; ptr++) cksum ^= *ptr;
    cksum ^= cxsec_key;
    end[-1] = cksum;
    return;
    }

void
cxsecVerifyDS(unsigned long* start, unsigned long* end, char* file, int line)
    {
    unsigned long* ptr;
    unsigned long cksum = 0;
    if (end[0] != (start[0] ^ cxsec_key))
	{
	printf("security violation: %s[%d]: ds verify failed\n", file, line);
	abort();
	}
    for(ptr=start; ptr<end; ptr++) cksum ^= *ptr;
    if (cksum != cxsec_key)
	{
	printf("security violation: %s[%d]: ds cksum verify failed\n", file, line);
	abort();
	}
    return;
    }

void
cxsecUpdateDS(unsigned long* start, unsigned long* end, char* file, int line)
    {
    unsigned long* ptr;
    unsigned long cksum = 0;
    if (end[0] != (start[0] ^ cxsec_key))
	{
	printf("security violation: %s[%d]: ds update failed\n", file, line);
	abort();
	}
    end[-1] = 0;
    for(ptr=start; ptr<end; ptr++) cksum ^= *ptr;
    cksum ^= cxsec_key;
    end[-1] = cksum;
    return;
    }

