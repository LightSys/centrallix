#ifndef _SMMALLOC_PRIVATE_H
#define _SMMALLOC_PRIVATE_H

#ifdef CXLIB_INTERNAL
#include "smmalloc.h"
#include "cxsec.h"
#else
#include "cxlib/smmalloc.h"
#include "cxlib/cxsec.h"
#endif

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2005 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	smmalloc.c, smmalloc.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 2005 					*/
/* Description: Shared memory segment memory manager.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: smmalloc_private.h,v 1.1 2005/02/26 04:32:59 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/smmalloc_private.h,v $

    $Log: smmalloc_private.h,v $
    Revision 1.1  2005/02/26 04:32:59  gbeeley
    - continued implementation of the shared-memory malloc module

    Revision 1.1  2005/02/06 05:08:01  gbeeley
    - Adding interface spec for shared memory management

 **END-CVSDATA***********************************************************/


/*** SmBlock - allocated or unallocated memory block ***/
typedef struct _SM_B
    {
    int		Magic;
    CXSEC_DS_BEGIN;
    pSmRegion	Region;
    pSmBlock	FreeBlkNext;
    pSmBlock	FreeBlkPrev;
    pSmBlock	BlkNext;
    pSmBlock	BlkPrev;
    size_t	Size;
    CXSEC_DS_END;
    }
    SmBlock, *pSmBlock;


/*** SmRegion - shared memory region ***/
struct _SM_T 
    {
    int		Magic;
    CXSEC_DS_BEGIN;
    SmHandle	Handle;
    SmHandle	IntHandle;		/* only used in some implementations */
    pSmBlock	FreeBlkHead;
    pSmBlock	FreeBlkTail;
    pSmBlock	BlkHead;
    pSmBlock	BlkTail;
    size_t	Size;
    size_t	AllocSize;
    int		nBlocks;
    int		nAllocBlocks;
    CXSEC_DS_END;
    };


#define SM_TOREL(r,p) ((void*)(((char*)p) - ((char*)r)))
#define SM_TOABS(r,p) ((void*)(((char*)r) + ((size_t)p)))

#endif /* not defined _SMMALLOC_PRIVATE_H */

