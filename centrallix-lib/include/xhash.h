#ifndef _XHASH_H
#define _XHASH_H


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

    $Id: xhash.h,v 1.1 2001/08/13 18:04:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xhash.h,v $

    $Log: xhash.h,v $
    Revision 1.1  2001/08/13 18:04:20  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/07/03 01:03:02  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#include "xarray.h"


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
int xhInit(pXHashTable this, int rows, int keylen);
int xhDeInit(pXHashTable this);
int xhAdd(pXHashTable this, char* key, char* data);
int xhRemove(pXHashTable this, char* key);
char* xhLookup(pXHashTable this, char* key);
int xhClear(pXHashTable this, int (*free_fn)());

#endif /* _XHASH_H */

