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
#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2017 LightSys Technology Services, Inc.		*/
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
/* Date:	December 13, 2017                                       */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.  This file provides a	*/
/*		PRNG stream sequence generator, not based on a random	*/
/*		pool.  The sequence is repeatable if initialized with	*/
/*		the same key each time.					*/
/************************************************************************/


/*** Algorithm:
 ***
 *** This keystream logic uses AES256 in Counter mode to generate the
 *** keystream, i.e. a stream cipher.
 ***
 *** The AES256 key is derived by SHA256 hashing the provided key.
 *** The AES256 IV is derived through a SHA256 hash of a transformation
 *** of the provided key.
 ***
 *** If the provided key is NULL, then a random key is obtained.
 ***/


/*** cxssKeystreamNew - Initialize the internal state and setup the keys and IV.
 ***/
pCxssKeystreamState
cxssKeystreamNew(unsigned char* key, int keylen)
    {
    pCxssKeystreamState kstate = NULL;
    unsigned char tmpkey[CXSS_CIPHER_STRENGTH];
    unsigned char tmphash[SHA256_DIGEST_LENGTH];
    unsigned char ivkey[CXSS_CIPHER_STRENGTH];
    int i;

	/** If the key is unset, use 256 bits from the random pool. **/
	if (!key)
	    {
	    cxssGenerateKey(tmpkey, sizeof(tmpkey));
	    key = tmpkey;
	    keylen = sizeof(tmpkey);
	    }

	/** Allocate the structures **/
	kstate = (pCxssKeystreamState)nmMalloc(sizeof(CxssKeystreamState));
	if (!kstate)
	    goto error;
	memset(kstate, 0, sizeof(CxssKeystreamState));
	kstate->Context = EVP_CIPHER_CTX_new();
	if (!kstate->Context)
	    {
	    mssError(1, "CXSS", "Could not create new cipher context via EVP_CIPHER_CTX_new()");
	    goto error;
	    }
	kstate->DataIndex = sizeof(kstate->Data); /* empty */

	/** Setup cipher key and IV from the provided key.  For the
	 ** cipher key, we just hash the provided key.  For the IV, we
	 ** derive it using a transformation and then hash the result,
	 ** truncating the hash (256 bit) to fit the IV (128 bit).
	 **/
	SHA256(key, keylen, kstate->Key);
	for(i=0; i<sizeof(ivkey); i++)
	    ivkey[i] = key[(sizeof(ivkey) - i - 1)%keylen];
	SHA256(ivkey, sizeof(ivkey), tmphash);
	memcpy(kstate->IV, tmphash, sizeof(kstate->IV));

	/** Initialize the AES256-CTR stream cipher to generate bytes
	 ** for us.
	 **/
	if (EVP_EncryptInit_ex(kstate->Context, EVP_aes_256_ctr(), NULL, kstate->Key, kstate->IV) != 1)
	    {
	    mssError(1, "CXSS", "Could not initialize AES256-CTR keystream via EVP_EncryptInit_ex()");
	    goto error;
	    }

	return kstate;

    error:
	/** Clean up **/
	if (kstate)
	    cxssKeystreamFree(kstate);
	return NULL;
    }


/*** cxssKeystreamGenerate - Generate a secure PRNG value of a given length
 ***/
int
cxssKeystreamGenerate(pCxssKeystreamState kstate, unsigned char* data, int datalen)
    {
    int ourlen;
    int cryptlen;
    unsigned char zero[CXSS_CIPHER_STRENGTH];

	/** Setup our plaintext (all zeros) **/
	memset(zero, 0, sizeof(zero));

	while (datalen > 0)
	    {
	    /** Do we have bytes available to use? **/
	    if (kstate->DataIndex < sizeof(kstate->Data))
		{
		ourlen = datalen;
		if (ourlen > sizeof(kstate->Data) - kstate->DataIndex)
		    ourlen = sizeof(kstate->Data) - kstate->DataIndex;
		memcpy(data, kstate->Data + kstate->DataIndex, ourlen);
		data += ourlen;
		datalen -= ourlen;
		kstate->DataIndex += ourlen;
		}

	    /** Fill another block? **/
	    if (datalen > 0)
		{
		if (EVP_EncryptUpdate(kstate->Context, kstate->Data, &cryptlen, zero, sizeof(zero)) != 1 || cryptlen != sizeof(kstate->Data))
		    {
		    mssError(1, "CXSS", "Could not generate AES256-CTR keystream via EVP_EncryptUpdate()");
		    goto error;
		    }
		kstate->DataIndex = 0;
		}
	    }

	return 0;

    error:
	return -1;
    }


/*** cxssKeystreamGenerateHex - generate a secure PRNG hex stream of the
 *** given length.  Note length == number of hex characters.  A nul byte
 *** is added after those characters.
 ***/
int
cxssKeystreamGenerateHex(pCxssKeystreamState kstate, char* hexdata, int hexdatalen)
    {

	/** Generate binary data **/
	if (cxssKeystreamGenerate(kstate, (unsigned char*)hexdata, hexdatalen/2+1) < 0)
	    goto error;

	/** Convert to hex **/
	if (cxss_i_Hexify((unsigned char*)hexdata, hexdatalen/2+1, hexdata, hexdatalen) < 0)
	    goto error;

	return hexdatalen;

    error:
	return -1;
    }


/*** cxssKeystreamFree - release keystream state information
 ***/
int
cxssKeystreamFree(pCxssKeystreamState kstate)
    {

	if (kstate->Context)
	    EVP_CIPHER_CTX_free(kstate->Context);
	memset(kstate, 0, sizeof(CxssKeystreamState));
	nmFree(kstate, sizeof(CxssKeystreamState));

    return 0;
    }

