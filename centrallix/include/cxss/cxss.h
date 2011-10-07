#ifndef _CXSS_H
#define _CXSS_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2007 LightSys Technology Services, Inc.		*/
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
/* Module:	cxss (security subsystem)                               */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	February 22, 2007                                       */
/*									*/
/* Description:	CXSS provides the core security services for the	*/
/*		Centrallix application platform.			*/
/************************************************************************/


#include <openssl/sha.h>

#define CXSS_ENTROPY_SIZE	1280

/*** CXSS data structures ***/
typedef struct _EP
    {
    int			PoolSize;		/* in bytes */
    int			StirCount;		/* # of occurrences */
    int			CurEstimate;		/* in bits */
    int			NextEstimate;		/* in bits */
    int			ReadPtr;		/* index into pool */
    int			NewBytesCount;		/* in bytes */
    unsigned char*	Pool;
    unsigned char*	BitsToAdd;
    unsigned char	XORkey[SHA_DIGEST_LENGTH];
    int			RunningDry;
    }
    CxssEntropyPool, *pCxssEntropyPool;


/*** CXSS module-wide data structure ***/
typedef struct _CXSS
    {
    CxssEntropyPool	Entropy;
    }
    CXSS_t;

extern CXSS_t CXSS;


/*** Main Security Subsystem functions ***/
int cxssInitialize();

/*** Utility functions ***/
int cxssGenerateKey(unsigned char* key, size_t n_bytes);
int cxssShred(unsigned char* data, size_t n_bytes);
int cxssAddEntropy(unsigned char* data, size_t n_bytes, int entropy_bits_estimate);

/*** Entropy functions - internal ***/
int cxss_internal_InitEntropy(int pool_size);
int cxss_internal_AddToPool(unsigned char* data, size_t n_bytes, int entropy_bits_estimate);
int cxss_internal_StirPool();
int cxss_internal_GetBytes(unsigned char* data, size_t n_bytes);

#endif /* not defined _CXSS_H */

