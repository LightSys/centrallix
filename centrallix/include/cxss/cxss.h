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
#include <openssl/ssl.h>
#include "cxlib/xarray.h"

#define CXSS_ENTROPY_SIZE	1280
#define CXSS_DEBUG_CONTEXTSTACK	1
#define CXSS_IDENTIFIER_LENGTH	64

#include "cxss/policy.h"

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


/*** Auth/Endorsement stack data structure ***/
typedef struct _AS
    {
    XArray		Endorsements;		/* array of pCxssEndorsement */
    XArray		SessionVariables;	/* array of pCxssVariable */
    struct _AS*		Prev;
    int			CopyCnt;		/* COW semantics */
#if CXSS_DEBUG_CONTEXTSTACK
    void*		CallerReturnAddr;	/* Used for warning of mismatched pop/push */
#endif
    }
    CxssCtxStack, *pCxssCtxStack;


/*** Session Variable ***/
typedef struct _CXSV
    {
    char		Name[CXSS_IDENTIFIER_LENGTH];
    char*		Value;			/* allocated with nmSysMalloc/nmSysStrdup */
    }
    CxssVariable, *pCxssVariable;


/*** Endorsement info ***/
typedef struct _EN
    {
    char		Endorsement[CXSS_IDENTIFIER_LENGTH];
    char		Context[CXSS_IDENTIFIER_LENGTH];
    }
    CxssEndorsement, *pCxssEndorsement;


/*** CXSS module-wide data structure ***/
typedef struct _CXSS
    {
    CxssEntropyPool	Entropy;
    pCxssPolicy		Policy;
    }
    CXSS_t;

extern CXSS_t CXSS;


/*** Main Security Subsystem functions ***/
int cxssInitialize();

/*** Utility functions ***/
int cxssGenerateKey(unsigned char* key, size_t n_bytes);
int cxssShred(unsigned char* data, size_t n_bytes);
int cxssAddEntropy(unsigned char* data, size_t n_bytes, int entropy_bits_estimate);

/*** Context/Authentication/Endorsement stack functions ***/
int cxssPopContext();
int cxssPushContext();
int cxssAddEndorsement(char* endorsement, char* context);
int cxssHasEndorsement(char* endorsement, char* context);
int cxssSetVariable(char* name, char* value, int valuealloc);
int cxssGetVariable(char* vblname, char** value, char* default_value);
int cxssGetEndorsementList(pXArray endorsements, pXArray contexts);

/*** Entropy functions - internal ***/
int cxss_internal_InitEntropy(int pool_size);
int cxss_internal_AddToPool(unsigned char* data, size_t n_bytes, int entropy_bits_estimate);
int cxss_internal_StirPool();
int cxss_internal_GetBytes(unsigned char* data, size_t n_bytes);

/*** TLS helper functions ***/
int cxssStartTLS(SSL_CTX* context, pFile* ext_conn, pFile* reporting_stream, int as_server);
int cxssFinishTLS(int childpid, pFile ext_conn, pFile reporting_stream);
int cxssStatTLS(pFile reporting_stream, char* status, int maxlen);

/*** Security Policy - Authorization API ***/
int cxssAuthorizeSpec(char* objectspec, int access_type, int log_mode);
int cxssAuthorize(char* domain, char* type, char* path, char* attr, int access_type, int log_mode);

#endif /* not defined _CXSS_H */

