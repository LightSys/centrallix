#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include "cxsec.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

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
/*		that are optional for application functioning, as well	*/
/*		as some just common sense stuff.			*/
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


/*** cxsecVerifySymbol() - makes sure that a string that is supposed to
 *** represent a symbol contains the correct types of characters. Returns
 *** -1 on failure, or 0 on success.
 ***
 *** In regex notation, this routine matches:
 ***
 ***     ^[A-Za-z_][A-Za-z0-9_]*$
 ***/
int
cxsecVerifySymbol(const char* sym)
    {

	/** First char must be alpha or underscore, and must exist (len >= 1).
	 ** We don't use isalpha() et al here because symbols need to conform to
	 ** the normal 'C' locale ascii charset!!!  To do otherwise can cause
	 ** significant security risks in the event of a locale mismatch!!
	 **/
	if (*sym != '_' && (*sym < 'A' || *sym > 'Z') && (*sym < 'a' || *sym > 'z'))
	    return -1;

	/** Next chars may be 1) end of string, 2) digits, 3) alpha, or 4) underscore **/
	sym++;
	while(*sym)
	    {
	    if (*sym != '_' && (*sym < 'A' || *sym > 'Z') && (*sym < 'a' || *sym > 'z') && (*sym < '0' || *sym > '9'))
		return -1;
	    sym++;
	    }

    return 0;
    }

int
cxsecVerifySymbol_n(const char* sym, size_t n)
    {

	/** First char must be alpha or underscore, and must exist (len >= 1).
	 ** We don't use isalpha() et al here because symbols need to conform to
	 ** the normal 'C' locale ascii charset!!!  To do otherwise can cause
	 ** significant security risks in the event of a locale mismatch!!
	 **/
	if (n <= 0 || (*sym != '_' && (*sym < 'A' || *sym > 'Z') && (*sym < 'a' || *sym > 'z')))
	    return -1;
	n--;

	/** Next chars may be 1) end of string, 2) digits, 3) alpha, or 4) underscore **/
	sym++;
	while(n)
	    {
	    if (*sym != '_' && (*sym < 'A' || *sym > 'Z') && (*sym < 'a' || *sym > 'z') && (*sym < '0' || *sym > '9'))
		return -1;
	    sym++;
	    n--;
	    }

    return 0;
    }

