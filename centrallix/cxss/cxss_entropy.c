#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxss/cxss.h"
#include <assert.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

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



/*** InitEntropy -- initialize the entropy pool.  Pool size MUST be a
 *** multiple of 20 bytes (160 bits), the SHA_DIGEST_LENGTH for SHA1.
 ***/
int cxss_internal_InitEntropy(int pool_size)
    {
    pFile fd;
    unsigned char data[16];
    struct timeval tm;
    int err = 0;
    pid_t pid;

	assert(pool_size % SHA_DIGEST_LENGTH == 0 && pool_size > 0);

	/** Initialize the pool data structure **/
	CXSS.Entropy.Pool = NULL;
	CXSS.Entropy.BitsToAdd = NULL;
	CXSS.Entropy.PoolSize = pool_size;
	CXSS.Entropy.Pool = nmMalloc(CXSS.Entropy.PoolSize);
	if (!CXSS.Entropy.Pool) 
	    {
	    err = -ENOMEM;
	    goto error;
	    }
	memset(CXSS.Entropy.Pool, 0, CXSS.Entropy.PoolSize);
	CXSS.Entropy.BitsToAdd = nmMalloc(CXSS.Entropy.PoolSize);
	if (!CXSS.Entropy.BitsToAdd)
	    {
	    err = -ENOMEM;
	    goto error;
	    }
	CXSS.Entropy.ReadPtr = 0;
	CXSS.Entropy.NewBytesCount = 0;
	CXSS.Entropy.StirCount = 0;
	CXSS.Entropy.CurEstimate = 0;
	CXSS.Entropy.NextEstimate = 0;
	CXSS.Entropy.RunningDry = 1;

	/** Add some initial entropy into the pool **/
	fd = fdOpen("/dev/urandom", O_RDONLY, 0600);
	if (fd)
	    {
	    /** Randomize the XOR key **/
	    if (fdRead(fd, (char*)CXSS.Entropy.XORkey, SHA_DIGEST_LENGTH, 0, FD_U_PACKET) != SHA_DIGEST_LENGTH)
		memset(CXSS.Entropy.XORkey, '\0', SHA_DIGEST_LENGTH);

	    /** Add 128 bits from /dev/urandom, if possible **/
	    if (fdRead(fd, (char*)data, sizeof(data), 0, FD_U_PACKET) == sizeof(data))
		{
		cxss_internal_AddToPool(data, sizeof(data), sizeof(data)*8);
		}
	    fdClose(fd, 0);

	    /** Add a few bits from the current time - and estimate
	     ** that there are 10 bits of entropy, for the milliseconds
	     ** part of the time.  The seconds part is included too, for
	     ** good measure, but we don't say it has any entropy.
	     **/
	    gettimeofday(&tm, NULL);
	    cxss_internal_AddToPool((unsigned char*)&tm, sizeof(tm), 10);

	    /** Add current process id, for good measure.  Since it is so
	     ** easily observed, we don't "say" it has any entropy.
	     **/
	    pid = getpid();
	    cxss_internal_AddToPool((unsigned char*)&pid, sizeof(pid), 0);
	    }

	/** Stir the pool a couple of times. **/
	cxss_internal_StirPool();
	cxss_internal_StirPool();

    return 0;

    error:
	if (CXSS.Entropy.Pool)
	    nmFree(CXSS.Entropy.Pool, CXSS.Entropy.PoolSize);
	CXSS.Entropy.Pool = NULL;
	if (CXSS.Entropy.BitsToAdd)
	    nmFree(CXSS.Entropy.BitsToAdd, CXSS.Entropy.PoolSize);
	CXSS.Entropy.BitsToAdd = NULL;
	CXSS.Entropy.PoolSize = 0;

    return err;
    }



/*** AddToPool - add some new bytes into the entropy pool.  Actually, this
 *** does not stir them in right away.  For efficiency, we squirrel them
 *** away to be stirred in when we need them, based on the entropy estimate
 *** and pool usage.
 ***/
int
cxss_internal_AddToPool(unsigned char* data, size_t n_bytes, int entropy_bits_estimate)
    {
    int byte_cnt = 0;
    int bytes_to_add;

	/** Add to pool, stirring if we end up filling up the new bytes buffer **/
	CXSS.Entropy.NextEstimate += entropy_bits_estimate;
	while(byte_cnt < n_bytes)
	    {
	    bytes_to_add = n_bytes - byte_cnt;
	    if (bytes_to_add > CXSS.Entropy.PoolSize - CXSS.Entropy.NewBytesCount)
		bytes_to_add = CXSS.Entropy.PoolSize - CXSS.Entropy.NewBytesCount;
	    memcpy(CXSS.Entropy.BitsToAdd + CXSS.Entropy.NewBytesCount, 
		    data + byte_cnt, bytes_to_add);
	    byte_cnt += bytes_to_add;
	    CXSS.Entropy.NewBytesCount += bytes_to_add;
	    if (CXSS.Entropy.NewBytesCount == CXSS.Entropy.PoolSize)
		cxss_internal_StirPool();
	    }

    return 0;
    }



/*** StirPool - take the data in the pool and stir it around to generate
 *** a fresh new entropy pool.
 ***/
int
cxss_internal_StirPool()
    {
    SHA_CTX ctx;
    int i;
    int rows;

	/** Step 1: XOR the new bytes into the pool **/
	if (CXSS.Entropy.NewBytesCount)
	    {
	    for(i=0;i<CXSS.Entropy.PoolSize;i++)
		{
		CXSS.Entropy.Pool[i] ^= CXSS.Entropy.BitsToAdd[i%CXSS.Entropy.NewBytesCount];
		}
	    }

	/** Step 2: Stir the pool using SHA1. **/
	rows = CXSS.Entropy.PoolSize/SHA_DIGEST_LENGTH;
	for(i=0;i<rows;i++)
	    {
	    SHA1_Init(&ctx);
	    SHA1_Update(&ctx, &i, sizeof(int));
	    SHA1_Update(&ctx, CXSS.Entropy.Pool+(i*SHA_DIGEST_LENGTH), SHA_DIGEST_LENGTH);
	    /*SHA1_Update(&ctx, CXSS.Entropy.Pool+((i+rows-1)%rows), SHA_DIGEST_LENGTH);
	    SHA1_Update(&ctx, CXSS.Entropy.Pool+((i+1)%rows), SHA_DIGEST_LENGTH);*/
	    SHA1_Update(&ctx, CXSS.Entropy.BitsToAdd, CXSS.Entropy.NewBytesCount);
	    SHA1_Update(&ctx, CXSS.Entropy.XORkey, SHA_DIGEST_LENGTH);
	    SHA1_Final(CXSS.Entropy.Pool+(i*SHA_DIGEST_LENGTH), &ctx);
	    }

	/** Step 3: Reset counters **/
	CXSS.Entropy.NewBytesCount = 0;
	CXSS.Entropy.ReadPtr = 0;
	CXSS.Entropy.StirCount++;
	CXSS.Entropy.CurEstimate += CXSS.Entropy.NextEstimate;
	if (CXSS.Entropy.CurEstimate > CXSS.Entropy.PoolSize*8)
	    CXSS.Entropy.CurEstimate = CXSS.Entropy.PoolSize*8;
	CXSS.Entropy.NextEstimate = 0;
	if (CXSS.Entropy.CurEstimate > 0) 
	    CXSS.Entropy.RunningDry = 0;

	/** Step 4: Regenerate the XOR key **/
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, CXSS.Entropy.Pool, CXSS.Entropy.PoolSize);
	SHA1_Update(&ctx, CXSS.Entropy.XORkey, SHA_DIGEST_LENGTH);
	SHA1_Final(CXSS.Entropy.XORkey, &ctx);

    return 0;
    }



/*** GetBytes - retrieve some random bytes from the entropy pool.
 ***/
int
cxss_internal_GetBytes(unsigned char* data, size_t n_bytes)
    {
    int byte_cnt = 0;

	while(byte_cnt < n_bytes)
	    {
	    /** Get a byte **/
	    data[byte_cnt] = CXSS.Entropy.XORkey[byte_cnt%SHA_DIGEST_LENGTH] ^ 
			     CXSS.Entropy.Pool[CXSS.Entropy.ReadPtr];
	    byte_cnt++;
	    CXSS.Entropy.ReadPtr++;

	    /** Did we exhaust the current pool? **/
	    if (CXSS.Entropy.ReadPtr == CXSS.Entropy.PoolSize)
		cxss_internal_StirPool();

	    /** Adjust the estimate **/
	    if (!CXSS.Entropy.RunningDry)
		CXSS.Entropy.CurEstimate -= 8;
	    else if (CXSS.Entropy.NextEstimate)
		cxss_internal_StirPool();

	    /** Did we exhaust the available entropy? **/
	    if (CXSS.Entropy.CurEstimate < 0)
		{
		/** Set status to running-dry, but stir the pool, some
		 ** more entropy might be available, and the pool should
		 ** be stirred anyhow
		 **/
		CXSS.Entropy.CurEstimate = 0;
		CXSS.Entropy.RunningDry = 1;
		cxss_internal_StirPool();
		}
	    }

    return 0;
    }

