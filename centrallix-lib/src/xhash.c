#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "xhash.h"
#include "xarray.h"
#include "newmalloc.h"

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

    $Id: xhash.c,v 1.1 2001/08/13 18:04:23 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/xhash.c,v $

    $Log: xhash.c,v $
    Revision 1.1  2001/08/13 18:04:23  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/07/03 01:02:57  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


/*** xh_internal_ComputeHash - calculates the hash value relative to the
 *** key and the number of rows in the hash table.
 ***/
int
xh_internal_ComputeHash(char* key, int keylen, int rows)
    {
    unsigned int v=0x53;
    int i;

	/** Run hash function on all bytes of key **/
	for(i=0;i<keylen;i++)
	    {
	    v = v<<3;
	    v = v^((unsigned char)(key[i]));
	    v = v^((v>>7)&0x26);
	    v = v<<3;
	    v = v^((unsigned char)(key[i]));
	    v = v^((v>>7)&0x45);
	    }

    return (v-1)%rows;
    }


/*** xhInit - initialize the hash table
 ***/
int 
xhInit(pXHashTable this, int rows, int keylen)
    {
    int i;

	/** Set up parameters. **/
	this->nRows = rows;
	this->KeyLen = keylen;
	this->nItems = 0;

	/** Initialize xarray **/
	xaInit(&(this->Rows),this->nRows);
	for(i=0;i<this->nRows;i++) xaAddItem(&(this->Rows),NULL);

    return 0;
    }


/*** xhDeInit - free up resources used by an initialized hash table.
 ***/
int 
xhDeInit(pXHashTable this)
    {

	/** Get rid of the xarray **/
	xaDeInit(&(this->Rows));

    return 0;
    }


/*** xhAdd - add an item to the hash table.  Duplicate keys are not
 *** permitted and will result in a return value of -1.
 ***/
int 
xhAdd(pXHashTable this, char* key, char* data)
    {
    pXHashEntry* e;
    pXHashEntry new;
    int t;
    int l=-1,kl;

	/** Check key length **/
	kl = this->KeyLen;
	if (kl == 0) kl = l = strlen(key);

	/** Find target **/
	t = xh_internal_ComputeHash(key, kl, this->nRows);
	e = (pXHashEntry*)(&(this->Rows.Items[t]));

	/** Check dup keys **/
	while(*e)
	    {
	    if (l>=0)
		{
		if (!strcmp((*e)->Key,key)) return -1;
		}
	    else
		{
		if (!memcmp((*e)->Key,key,kl)) return -1;
		}
	    e = &((*e)->Next);
	    }

	/** Add item **/
	new = (pXHashEntry)nmMalloc(sizeof(XHashEntry));
	if (!new) return -1;
	*e = new;
	new->Key = key;
	new->Data = data;
	new->Next = NULL;

	/** Bump cnt **/
	this->nItems++;

    return 0;
    }


/*** xhRemove - remove an existing item from the hash table.  If the
 *** item does not exist, -1 is returned.
 ***/
int 
xhRemove(pXHashTable this, char* key)
    {
    pXHashEntry* e;
    pXHashEntry del;
    int t;
    int l=-1,kl;

	/** Check key length **/
	kl = this->KeyLen;
	if (kl == 0) kl = l = strlen(key);

	/** Find target **/
	t = xh_internal_ComputeHash(key, kl, this->nRows);
	e = (pXHashEntry*)(&(this->Rows.Items[t]));

	/** Search for it. **/
	while(*e)
	    {
	    if ((l==-1 && !memcmp((*e)->Key,key,kl)) ||
		(l>=0 && !strcmp((*e)->Key,key)))
		{
		del = (*e);
		(*e) = (*e)->Next;
		nmFree(del,sizeof(XHashEntry));
		this->nItems--;
		return 0;
	        }
	    e = &((*e)->Next);
	    }

    return -1;
    }


/*** xhLookup - find an entry in the hash table by looking up via its
 *** key.
 ***/
char* 
xhLookup(pXHashTable this, char* key)
    {
    pXHashEntry* e;
    int t;
    int l=-1,kl;

	/** Check key length **/
	kl = this->KeyLen;
	if (kl == 0) kl = l = strlen(key);

	/** Find target **/
	t = xh_internal_ComputeHash(key, kl, this->nRows);
	e = (pXHashEntry*)(&(this->Rows.Items[t]));

	/** Search for it. **/
	while(*e)
	    {
	    if ((l==-1 && !memcmp((*e)->Key,key,kl)) ||
		(l>=0 && !strcmp((*e)->Key,key)))
		{
		return (*e)->Data;
	        }
	    e = &((*e)->Next);
	    }
    
    return NULL;
    }


/*** xhClear - clears all contents from a hash table, optionally
 *** freeing the data as memory blocks.
 ***/
int 
xhClear(pXHashTable this, int (*free_fn)())
    {
    int i;
    pXHashEntry e,del;

	/** Clear each item. **/
	for(i=0;i<this->nRows;i++) 
	    {
	    e=(pXHashEntry)(this->Rows.Items[i]);
	    while(e)
		{
		del = e;
		if (free_fn) free_fn(e->Data);
		e=e->Next;
		nmFree(del,sizeof(XHashEntry));
		}
	    this->Rows.Items[i] = NULL;
	    }

	/** Clear count **/
	this->nItems = 0;

    return 0;
    }


