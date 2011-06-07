#ifndef _XHASH_H
#define _XHASH_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	xhash.c, xhash.h  					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 12, 1998					*/
/* Description:	Extensible hash table system for Centrallix.  This	*/
/*		module allows for key/value pairing with fast lookup	*/
/*		of the value by the key.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xhash.h,v 1.3 2005/02/26 04:32:02 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xhash.h,v $

    $Log: xhash.h,v $
    Revision 1.3  2005/02/26 04:32:02  gbeeley
    - moving include file install directory to include a "cxlib/" prefix
      instead of just putting 'em all in /usr/include with everything else.

    Revision 1.2  2002/05/03 03:46:29  gbeeley
    Modifications to xhandle to support clearing the handle list.  Added
    a param to xhClear to provide support for xhnClearHandles.  Added a
    function in mtask.c to allow the retrieval of ticks-since-boot without
    making a syscall.  Fixed an MTASK bug in the scheduler relating to
    waiting on timers and some modulus arithmetic.

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:02  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/

#ifdef CXLIB_INTERNAL
#include "xarray.h"
#else
#include "cxlib/xarray.h"
#endif


/** Structures **/
typedef struct _XE
    {
    struct _XE*	Next;
    char*	Data;
    char*	Key;
    }
    XHashEntry, *pXHashEntry;

#define XHE(x) ((pXHashEntry)(x))

typedef struct 
    {
    int		nRows;
    XArray	Rows;
    int		KeyLen;
    int		nItems;
    }
    XHashTable, *pXHashTable;

/** Functions **/
int xhInit(pXHashTable, int rows, int keylen);
int xhDeInit(pXHashTable);
int xhAdd(pXHashTable, char* key, char* data);
int xhRemove(pXHashTable, char* key);
char* xhLookup(pXHashTable, char* key);
int xhClear(pXHashTable, int (*free_fn)(), void* free_arg);

#ifdef __cplusplus
}
#endif

#endif /* _XHASH_H */

