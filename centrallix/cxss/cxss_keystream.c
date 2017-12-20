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


/*** noncebuf
 ***
 ***     [current sha256 hash][counter][original sha256 hash]
 ***
 *** initialization
 ***
 ***     1. If key not provided, use 32 bytes from random pool for key
 ***     2. Hash key, place in current and original hashes
 ***     3. Set counter to zero
 ***
 *** algorithm
 ***
 ***     1. Hash the noncebuf, result into current sha256 hash
 ***     2. Increment counter
 ***     3. Return required bytes from current sha256 hash
 ***     4. If more bytes needed, repeat the process
 ***/


/*** cxssKeystreamNew - Initialize the internal state and setup the keys and IV.
 ***/
pCxssKeystreamState
cxssKeystreamNew(unsigned char* key, int keylen)
    {
    pCxssKeystreamState kstate;
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
	    goto error;
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
	    goto error;

	return kstate;

    error:
	/** Clean up **/
	if (kstate)
	    {
	    if (kstate->Context)
		EVP_CIPHER_CTX_free(kstate->Context);
	    memset(kstate, 0, sizeof(CxssKeystreamState));
	    nmFree(kstate, sizeof(CxssKeystreamState));
	    }
	return NULL;
    }


/*** cxssKeystreamGenerate - Generate a nonce value of a given length
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
		    return -1;
		kstate->DataIndex = 0;
		}
	    }

    return 0;
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

