#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
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


#if 0
int n_hash_adds = 0;
int n_hash_collisions = 0;
#endif



unsigned int xh_random[256] =
    {
    358213, 149183, 380867, 117727, 205453, 351859, 157247, 282911, 
    168481, 97549, 314641, 29401, 93629, 86399, 196159, 107687, 
    312343, 339907, 361843, 293269, 16493, 54673, 21019, 274783, 
    129629, 289847, 249853, 127321, 29581, 122719, 27631, 237911, 
    150991, 75991, 380839, 186317, 134681, 271043, 104623, 195229, 
    58061, 31379, 184511, 209477, 134947, 194723, 160073, 184073, 
    342659, 147209, 291101, 278581, 356143, 19841, 380333, 243701, 
    99907, 70099, 90263, 1231, 51929, 327571, 285031, 300331, 
    10007, 138107, 358753, 2707, 49529, 234529, 50989, 161303, 
    68449, 368857, 3779, 107761, 264463, 318377, 197753, 314137, 
    320141, 316037, 354643, 225821, 53281, 176933, 287341, 375757, 
    368597, 36919, 179383, 339323, 230203, 340777, 27803, 326113, 
    358301, 92767, 211283, 135899, 179909, 71909, 185947, 95483, 
    92419, 36529, 3461, 176329, 170767, 376237, 136963, 264031, 
    276151, 281923, 2293, 167677, 317419, 125887, 182387, 357241, 
    328651, 109819, 322999, 29759, 50123, 160627, 174337, 323137, 
    249833, 1787, 51859, 134707, 106109, 215261, 336593, 308309, 
    341057, 233417, 307189, 93901, 364223, 332011, 98627, 152833, 
    259169, 137873, 311111, 80783, 53777, 278717, 121883, 362443, 
    261823, 267481, 263009, 189697, 89627, 329297, 151153, 203909, 
    219829, 138077, 266899, 295567, 233983, 164581, 257069, 62627, 
    51721, 201953, 265271, 12697, 72907, 373553, 359663, 228539, 
    50993, 346589, 6977, 331307, 32467, 92233, 378997, 40213, 
    22247, 178513, 314453, 196051, 88471, 243479, 251707, 7559, 
    174599, 42281, 3529, 134923, 105397, 123593, 224771, 289469, 
    17123, 115057, 17881, 272059, 284737, 78781, 321077, 223211, 
    171559, 252409, 59359, 342343, 109133, 368789, 222977, 153337, 
    184271, 376633, 189887, 361451, 300163, 8819, 248021, 349133, 
    235447, 79601, 277657, 73757, 139883, 280811, 36161, 191861, 
    6361, 198469, 363611, 332309, 141223, 378733, 176651, 167801, 
    336263, 305839, 11173, 44909, 61543, 340979, 95881, 143947, 
    60217, 133351, 141761, 225221, 250993, 167593, 317431, 52697, 
    };


/*** xh_internal_ComputeHash - calculates the hash value relative to the
 *** key and the number of rows in the hash table.
 ***/
int
xh_internal_ComputeHash(char* key, int keylen, int rows)
    {
    unsigned int v=1;
    int i;

	/** Run hash function on all bytes of key **/
	for(i=0;i<keylen;i++)
	    {
	    v = v+(xh_random[((unsigned char)(key[i]) + v)&0xFF]);
	    }

    return (v>>1)%rows;
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
#if 0
	n_hash_adds++;
	if (e != (pXHashEntry*)(&(this->Rows.Items[t]))) 
	    {
	    n_hash_collisions++;
	    if (l>=0) printf("Hash: collision ts=%d t=%d k1='%s' k2='%s'\n", this->nRows, t, ((pXHashEntry)(this->Rows.Items[t]))->Key, key);
	    }
#endif

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
xhClear(pXHashTable this, int (*free_fn)(), void* free_arg)
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
		if (free_fn) free_fn(e->Data, free_arg);
		e=e->Next;
		nmFree(del,sizeof(XHashEntry));
		}
	    this->Rows.Items[i] = NULL;
	    }

	/** Clear count **/
	this->nItems = 0;

    return 0;
    }


