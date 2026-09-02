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
int xhInit(pXHashTable this, int rows, int keylen);
int xhDeInit(pXHashTable this);
int xhAdd(pXHashTable this, char* key, char* data);
int xhRemove(pXHashTable this, char* key);
char* xhLookup(pXHashTable this, char* key);
int xhClear(pXHashTable this, int (*free_fn)(), void* free_arg);

#endif /* _XHASH_H */

