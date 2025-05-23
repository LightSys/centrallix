#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxss/cxss.h"
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <ctype.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2024 LightSys Technology Services, Inc.		*/
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
/* Date:	March 27, 2024                                          */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.  This file provides	*/
/*		the ability to sign and verify links, to work around 	*/
/*		same-origin issues with generated documents.		*/
/************************************************************************/

static struct
    {
    pCxssKeystreamState		KeystreamState;
    unsigned char*		Key;
    int				KeyLength;
    pXArray			SiteList;
    }
    CXSS_LINKSIGN = { .KeystreamState = NULL, .Key = NULL, .KeyLength=0, .SiteList=NULL };


/*** cxssLinkHash - computes a URL signing hash.  Key should be in binary
 *** form, but nonce should be in hex form.  Hash is returned in hex form.
 *** This function does not include the https://server/ part of the URL in
 *** the hash.
 ***/
int
cxssLinkHash(char* url, int urllen, unsigned char* key, int keylen, char* nonce, int noncelen, char* hash, int hashlen)
    {
    pXString hashstr = NULL;
    unsigned char binhashvalue[EVP_MAX_MD_SIZE];
    unsigned int binhashlen;
    char hex[] = "0123456789abcdef";
    int i;
    char* pathptr;

	/** Build the hashable string **/
	hashstr = xsNew();
	if (!hashstr)
	    goto error;
	pathptr = strstr(url, "//");
	if (!pathptr)
	    {
	    pathptr = url;
	    }
	else if (strchr(pathptr+2, '/'))
	    {
	    pathptr = strchr(pathptr+2, '/');
	    }
	xsConcatenate(hashstr, pathptr, urllen - (pathptr - url));
	xsConcatenate(hashstr, nonce, noncelen);

	/** Hash it **/
	HMAC(EVP_sha256(), key, keylen, (unsigned char*)xsString(hashstr), xsLength(hashstr), binhashvalue, &binhashlen);

	/** Convert to hex **/
	if (hashlen < binhashlen*2 + 1)
	    goto error;
	for(i=0; i<binhashlen; i++)
	    {
	    hash[i*2] = hex[(binhashvalue[i] & 0xf0)>>4];
	    hash[i*2+1] = hex[(binhashvalue[i] & 0x0f)];
	    }
	hash[binhashlen*2] = '\0';

	xsFree(hashstr);

	return 0;

    error:
	if (hashstr)
	    xsFree(hashstr);
	return -1;
    }


/*** cxssLinkSign - signs a URL by adding a parameter to it that contains both
 *** a nonce and a HMAC.
 ***
 *** Format:  https://server/path?a=b&cx__lkey=[nonce][hash]
 ***/
pXString
cxssLinkSign(char* url)
    {
    pXString signed_link = NULL;
    char hash[EVP_MAX_MD_SIZE * 2 + 1];
    char nonce[17];
    int i, found, len, sitelen;
    char* site;

	/** Initialized? **/
	if (!CXSS_LINKSIGN.KeystreamState)
	    goto error;

	len = strlen(url);

	/** Is this a site that we'll sign? **/
	for(found=i=0; i<CXSS_LINKSIGN.SiteList->nItems; i++)
	    {
	    site = (char*)CXSS_LINKSIGN.SiteList->Items[i];
	    sitelen = strlen(site);
	    if (sitelen > 0 && (!strcmp(site, "*") || (len >= sitelen && !strncmp(url, site, sitelen))))
		{
		/** Site is non-empty, and is either * or a prefix of the url **/
		found = 1;
		break;
		}
	    }
	if (!found)
	    goto error;

	/** Create the nonce **/
	signed_link = xsNew();
	if (!signed_link)
	    goto error;
	if (cxssKeystreamGenerateHex(CXSS_LINKSIGN.KeystreamState, nonce, 16) < 0)
	    goto error;

	/** Create the HMAC **/
	if (cxssLinkHash(url, len, CXSS_LINKSIGN.Key, CXSS_LINKSIGN.KeyLength, nonce, 16, hash, sizeof(hash)) < 0)
	    goto error;

	/** Build the new signed link **/
	xsQPrintf(signed_link, "%STR%STRcx__lkey=%STR&URL%STR&URL", 
		url,
		strchr(url, '?')?"&":"?",
		nonce,
		hash
		);

	return signed_link;

    error:
	if (signed_link)
	    xsFree(signed_link);
	return NULL;
    }


/*** cxssLinkVerify - takes a possibly-signed link and verifies the signature.
 *** Returns -2 on verification failure, -1 if there is no signature, and 0
 *** if the signature verifies correctly.
 ***
 *** URL must contain ?cx__lkey= or &cx__lkey= followed by the hex nonce and
 *** then followed by the hex HMAC.  Everything before that string will be
 *** included in the HMAC calculation.
 ***/
int
cxssLinkVerify(char* url)
    {
    char hash[EVP_MAX_MD_SIZE * 2 + 1];
    char* hmacptr;

	if (!CXSS_LINKSIGN.KeystreamState)
	    goto error;

	/** Pick apart the url **/
	hmacptr = strstr(url, "cx__lkey=");
	if (!hmacptr || hmacptr == url)
	    goto error;
	if (hmacptr[-1] != '?' && hmacptr[-1] != '&')
	    goto error;

	/** Compute the correct HMAC **/
	if (cxssLinkHash(url, hmacptr - url - 1, CXSS_LINKSIGN.Key, CXSS_LINKSIGN.KeyLength, hmacptr + 9, 16, hash, sizeof(hash)) < 0)
	    goto error;

	/** Was the correct one supplied in the url? **/
	if (strcmp(hash, hmacptr + 9 + 16) != 0)
	    goto error;

	return 0;

    error:
	return -1;
    }


/*** cxssLinkInitialize - set the key used for signing and verification of 
 *** links and initialize the link verification system.
 ***/
int
cxssLinkInitialize(unsigned char* key, int keylen, pXArray site_list)
    {

	if (keylen <= 0)
	    {
	    mssError(1, "CXSS", "Invalid link key length %d", keylen);
	    goto error;
	    }
	CXSS_LINKSIGN.KeyLength = keylen;
	CXSS_LINKSIGN.Key = nmSysMalloc(keylen);
	if (!CXSS_LINKSIGN.Key)
	    goto error;
	memcpy(CXSS_LINKSIGN.Key, key, keylen);
	CXSS_LINKSIGN.KeystreamState = cxssKeystreamNew(NULL, 0);
	if (!CXSS_LINKSIGN.KeystreamState)
	    goto error;
	CXSS_LINKSIGN.SiteList = site_list;

	return 0;

    error:
	return -1;
    }

