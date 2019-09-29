#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2019 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	b+tree.c, b+tree.h  					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 11, 2019					*/
/* Description:	B+ Tree implementation.					*/
/************************************************************************/


/*** bptNew() - allocate and initialize a new B+ Tree
 ***/
pBPTree
bptNew()
    {
    pBPTree this;

	/** Allocate **/
	this = (pBPTree)nmMalloc(sizeof(BPTree));
	if (!this)
	    return NULL;

	/** Init **/
	if (bptInit(this) != 0)
	    {
	    nmFree(this, sizeof(BPTree));
	    return NULL;
	    }

    return this;
    }


/*** bptInit() - initialize an already-allocated B+ Tree
 ***/
int
bptInit(pBPTree this)
    {

	/** Clear out the data structure **/
	this->Parent = this->Next = this->Prev = NULL;
	this->nKeys = 0;
	this->IsLeaf = 1;

    return 0;
    }


/*** bptFree() - deinit and deallocate a B+ Tree
 ***/
void
bptFree(pBPTree this)
    {

	bptDeInit(this);
	nmFree(this, sizeof(BPTree));

    return;
    }


/*** bptDeInit() - deinit a B+ Tree, but don't deallocate
 ***/
int
bptDeInit(pBPTree this)
    {
    int i;

	/** Deallocate children **/
	if (!this->IsLeaf)
	    {
	    for(i=0; i<this->nKeys+1; i++)
		{
		bptFree(this->Children[i].Child);
		}
	    }

	/** Deallocate key values **/
	for(i=0; i<this->nKeys; i++)
	    {
	    nmSysFree(this->Keys[i].Value);
	    }
	this->nKeys = 0;

	this->Parent = this->Next = this->Prev = NULL;

    return 0;
    }


/*** bpt_i_Compare() - compares two key values.  Return value is greater
 *** than 0 if key1 > key2, less than zero if key1 < key2, and equal to
 *** zero if key1 == key2.
 ***/
int
bpt_i_Compare(char* key1, int key1_len, char* key2, int key2_len)
    {
    int len, rval;

	/** Common length **/
	if (key1_len > key2_len)
	    len = key2_len;
	else
	    len = key1_len;

	/** Compare **/
	rval = memcmp(key1, key2, len);

	/** Initial parts same: compare based on lengths of keys. **/
	if (rval == 0)
	    rval = key1_len - key2_len;

    return rval;
    }


/*** bpt_i_Split() - split a node.  Returns the new node.  The new node is
 *** always to the right of the current node (the second half of the keys/values
 *** are moved from the current node to the new node).
 ***/
pBPTree
bpt_i_Split(pBPTree this)
    {
    }


/*** bpt_i_Push() - push a value to the left or right sibling
 ***/
int
bpt_i_Push(pBPTree this)
    {
    }


/*** bpt_i_Scan() - scan a node's keys looking for a key
 ***/
int
bpt_i_Scan(pBPTree this, char* key, int key_len, int *locate_index)
    {
    int i, rval;

	/** Find it **/
	for(i=0; i<this->nKeys; i++)
	    {
	    rval = bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length);
	    if (rval <= 0)
		break;
	    }

	*locate_index = i;

    return rval;
    }


/*** bpt_i_Find() - locate a key/value pair in the tree.  If not
 *** found, determine the location where it should be inserted.  Returns
 *** 0 if found, -1 if not found.
 ***/
int
bpt_i_Find(pBPTree this, char* key, int key_len, pBPTree *locate, int *locate_index)
    {
    int rval;

	/** Scan this node **/
	rval = bpt_i_Scan(this, key, key_len, locate_index);
	if (this->IsLeaf)
	    {
	    *locate = this;
	    return rval;
	    }

	/** Scan the selected child node **/
	rval = bpt_i_Find(this->Children[*locate_index].Child, key, key_len, locate, locate_index);

    return rval;
    }


/*** bpt_i_LeafInsert() - insert a key/value pair into a leaf node.  Does NOT check
 *** to see if there is room - the caller must do that first.
 ***/
int
bpt_i_LeafInsert(pBPTree this, char* key, int key_len, void* data, int idx)
    {
    void* copy;

	/** Make a copy of the key **/
	copy = nmSysMalloc(key_len);
	if (!copy)
	    return -1;
	memcpy(copy, key, key_len);

	/** Make room for the key and value **/
	if (idx != this->nKeys)
	    {
	    memmove(this->Keys+idx+1, this->Keys+idx, sizeof(BPTreeKey) * (this->nKeys - idx));
	    memmove(this->Children+idx+1, this->Children+idx, sizeof(BPTreeVal) * (this->nKeys - idx));
	    this->nKeys++;
	    }

	/** Set it **/
	this->Keys[idx].Length = key_len;
	this->Keys[idx].Value = copy;
	this->Children[idx].Ref = data;

    return 0;
    }


/*** bptAdd() - add a key/value pair to the tree.  Returns 1 if the
 *** key/value pair already exists, 0 on success, or -1 on error.
 ***/
int
bptAdd(pBPTree this, char* key, int key_len, void* data)
    {
    pBPTree node, new_node = NULL;
    int idx;

	/** See if it is there. **/
	if (bpt_i_Find(this, key, key_len, &node, &idx) == 0)
	    {
	    /** Already exists.  Don't add. **/
	    return 1;
	    }

	/** Not enough room? **/
	if (node->nKeys == BPT_SLOTS)
	    {
	    new_node = bpt_i_Split(node);

	    /** Error condition if new_node is NULL. **/
	    if (!new_node)
		return -1;
	    }

	/** Which node are we adding to? **/
	if (new_node)
	    {
	    if (idx > BPT_SLOTS / 2)
		{
		node = new_node;
		idx -= (BPT_SLOTS / 2);
		}
	    }

	/** Insert the item **/
	if (bpt_i_LeafInsert(node, key, key_len, data, idx) < 0)
	    return -1;

    return 0;
    }


/*** bptLookup() - find a value for a given key.  Returns NULL if the
 *** key does not exist in the tree
 ***/
void*
bptLookup(pBPTree this, char* key, int key_len)
    {
    pBPTree node;
    int idx;

	/** Look it up **/
	if (bpt_i_Find(this, key, key_len, &node, &idx) == 0)
	    {
	    /** Found **/
	    return node->Children[idx].Ref;
	    }

    return NULL;
    }


/*** bptRemove() - removes a key/value pair from the tree.  Returns -1
 *** if the key was not found.
 ***/
int
bptRemove(pBPTree this, char* key, int key_len)
    {
    }


/*** bptClear() - clears the tree of all key/value pairs.  Calls the provided
 *** free_fn for each key/value pair, as:  free_fn(free_arg, value)
 ***/
int
bptClear(pBPTree this, int (*free_fn)(), void* free_arg)
    {
    int i;

	/** Clear child subtrees first **/
	for(i=0; i<this->nKeys; i++)
	    {
	    if (this->IsLeaf)
		{
		free_fn(free_arg, this->Children[i].Ref);
		}
	    else
		{
		bptClear(this->Children[i].Child, free_fn, free_arg);
		nmFree(this->Children[i].Child, sizeof(BPTree));
		}
	    }
	if (!this->IsLeaf)
	    {
	    bptClear(this->Children[this->nKeys].Child, free_fn, free_arg);
	    nmFree(this->Children[this->nKeys].Child, sizeof(BPTree));
	    }

	/** Clear key/value pairs **/
	for(i=0; i<this->nKeys; i++)
	    {
	    nmSysFree(this->Keys[i].Value);
	    free_fn(free_arg, this->Keys[i].Key);
	    }

	/** Reset the root node **/
	if (!this->Parent)
	    bptInit(this);

    return 0;
    }

