// TODO use shortened keys?
// TODO figure out what we want bptSearch to return (other than NULL for not found)
// TODO set prev and next pointers

#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "b+tree.h"
#include "newmalloc.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2020 LightSys Technology Services, Inc.		*/
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
BPTree*
bptNew()
    {
    BPTree* this;

    /** Allocate **/
    this = nmMalloc(sizeof(BPTree));
    if (!this)
        return NULL;

    this->root = bpt_i_new_BPNode();
    if (!this->root)
        return NULL;

    /** Init **/
    if (bptInit(this->root) != 0)
        {
        nmFree(this->root, sizeof(BPNode));
        nmFree(this, sizeof(BPTree));
        return NULL;
        }

    return this;
    }

/*** bptInsert(T, k, v) - insert k,v into tree T in a single pass down the tree ***/
int bptInsert(BPTree* this, char* key, int key_len, void* data)
    {
    if (bptSearch(this->root, key, key_len) != NULL)
        {
        return -1;  // key already exists; TODO the behavior for dup-insert-attempt should be defined somewhere
        }
    
    if (this->root->nKeys == 2 * T_SLOTS - (this->root->IsLeaf ? 0 : 1))   // full

        {
        pBPNode newRoot = bpt_i_new_BPNode();
        if (!newRoot) return -1;
        newRoot->IsLeaf = 0;
        newRoot->Children[0].Child = this->root;
        this->root = newRoot;

        if (bpt_i_Split_Child(newRoot, 0) < 0) return -1;
        }
    if (bpt_i_Insert_Nonfull(this->root, key, key_len, data) < 0) return -1;

    return 0;
    }

/*** Return pointer to newly allocated and initialized BPNode, or NULL if fails */
pBPNode 
bpt_i_new_BPNode()
    {
        pBPNode ret = (pBPNode)nmMalloc(sizeof(BPNode));
        if (!ret)
            return NULL;
        if (bptInit(ret) != 0)
            {
            nmFree(ret, sizeof(pBPNode));
            return NULL;
            }

        return ret;
    }

/*** Given nonfull internal node this and index such that this.Children[index] is full,
 *   split this.Children[index] and adjust this to have an additional child
 ***/
int
bpt_i_Split_Child(pBPNode this, int index)
    {
    assert (this->nKeys < 2 * T_SLOTS - (this->IsLeaf ? 0 : 1));   // nonfull
    pBPNode oldChild = this->Children[index].Child;
    assert (oldChild->nKeys == 2 * T_SLOTS - (oldChild->IsLeaf ? 0 : 1));   // full
    pBPNode newChild = bpt_i_new_BPNode();
        if (!newChild) return -1;
    newChild->IsLeaf = oldChild->IsLeaf;
    newChild->nKeys = T_SLOTS - (oldChild->IsLeaf ? 0 : 1);
    oldChild->nKeys = T_SLOTS - (oldChild->IsLeaf ? 0 : 1);

    int i;

    if (oldChild->IsLeaf) 
        {
        for (i = 0; i < newChild->nKeys; i++)
            {
            newChild->Keys[i].Value = oldChild->Keys[i+newChild->nKeys].Value;
            newChild->Keys[i].Length = oldChild->Keys[i+newChild->nKeys].Length;
            newChild->Children[i].Ref = oldChild->Children[i+oldChild->nKeys].Ref;
            }
        }
    else 
        {
        for (i = 0; i < newChild->nKeys; i++)
            {
            newChild->Keys[i].Value = oldChild->Keys[i+newChild->nKeys+1].Value;
            newChild->Keys[i].Length = oldChild->Keys[i+newChild->nKeys+1].Length;
            }
        for (i = 0; i <= newChild->nKeys; i++)
            newChild->Children[i].Child = oldChild->Children[i+oldChild->nKeys+1].Child;
        }

    for (i=this->nKeys; i > index; i--)
        {
        this->Children[i+1].Child = this->Children[i].Child;
        }
    this->Children[index+1].Child = newChild;

    for (i=this->nKeys; i >= index; i--)
        {
        this->Keys[i+1].Value = this->Keys[i].Value;
        this->Keys[i+1].Length = this->Keys[i].Length;
        }
    this->Keys[index].Value = oldChild->Keys[T_SLOTS-(oldChild->IsLeaf ? 0 : 1)].Value;
    this->Keys[index].Length = oldChild->Keys[T_SLOTS-(oldChild->IsLeaf ? 0 : 1)].Length;

    this->nKeys++;
    return 0;
    }

/*** Inserts key,data into node "this", assumed to be nonfull ***/
int
bpt_i_Insert_Nonfull(pBPNode this, char* key, int key_len, void* data)
    {
    while (1)
        {
        assert (this->nKeys < 2 * T_SLOTS - (this->IsLeaf ? 0 : 1));   // nonfull
        int i = this->nKeys-1;
        
        if (this->IsLeaf) 
            {
            while (i >= 0 && bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) < 0) 
                {
                this->Keys[i+1].Value = this->Keys[i].Value;
                this->Keys[i+1].Length = this->Keys[i].Length;
                this->Children[i+1].Ref = this->Children[i].Ref;
                i--;
                }
            this->Keys[i+1].Value = malloc(key_len);
            if (!this->Keys[i+1].Value)
                {
                return -1;
                }
            memcpy(this->Keys[i+1].Value, key, key_len);
            this->Keys[i+1].Length = key_len;
            this->Children[i+1].Ref = data;
            this->nKeys++;
            return 0;
            }
        else
            {
            while (i >= 0 && bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) < 0) 
                {
                i--;
                }
            i++;
            if (this->Children[i].Child->nKeys == 2 * T_SLOTS - (this->Children[i].Child->IsLeaf ? 0 : 1))   // full
                {
                if (bpt_i_Split_Child(this, i) < 0) return -1;
                if (bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) > 0) i++;
                }
            
            this = this->Children[i].Child;
            }
        }
    }

int bptRemove(BPTree* tree, char* key, int key_len, int (*free_fn)(), void* free_arg)
    {
    if (bptSearch(tree->root, key, key_len) == NULL) return -1;
    pBPNode this = tree->root;
    pBPNode parent = NULL;
    int thisIndex;
    while (1)
        {
        // is key in this?
        int i = 0;
        int cmp;
        for (i=0; i<this->nKeys; i++)
            {
            cmp = bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length);
            if (cmp <= 0) break;
            }

        // if cmp == 0, this->Keys[i].Value == key
        // else if this->IsLeaf, not found
        // else this->Children[i + (cmp>0 ? 1 : 0)].Child contains key (if key is present)
        if (cmp == 0)   // found key in this at index i
            {
            if (this->IsLeaf)
                {
                // free child
                nmFree(this->Keys[i].Value, this->Keys[i].Length);
                free_fn(free_arg, this->Children[i].Ref);
                // delete key from this
                for (; i<this->nKeys-1; i++)
                    {
                    this->Keys[i].Value = this->Keys[i+1].Value;
                    this->Keys[i].Length = this->Keys[i+1].Length;
                    this->Children[i].Ref = this->Children[i+1].Ref;
                    }
                this->nKeys--;
                return 0;
                }
            else
                {
                pBPNode searchNext = this->Children[i+1].Child;
                pBPNodeKey newKey = bpt_i_FindReplacementKey(this, key, key_len);
                if (searchNext->nKeys >= T_SLOTS)     
                    {
                    this->Keys[i].Value = newKey->Value;
                    this->Keys[i].Length = newKey->Length;
                    }
                else
                    {
                    pBPNode prev = this->Children[i].Child;
                    pBPNode next = (i+2 <= this->nKeys ? this->Children[i].Child : NULL);
                    
                    if (prev->nKeys >= T_SLOTS)                    
                        {
                        // move a key/child from prev to searchNext
                        int j;
                        for (j=searchNext->nKeys-1; j>=0; j--)
                            {
                            searchNext->Keys[j+1].Value = searchNext->Keys[j].Value;
                            searchNext->Keys[j+1].Length = searchNext->Keys[j].Length;
                            }
                        if (searchNext->IsLeaf)
                            {
                            for (j=searchNext->nKeys-1; j>=0; j--)
                                {
                                searchNext->Children[j+1].Ref = searchNext->Children[j].Ref;
                                }
                            searchNext->Children[0].Ref = prev->Children[prev->nKeys-1].Ref;
                            searchNext->Keys[0].Value = prev->Keys[prev->nKeys-1].Value;
                            searchNext->Keys[0].Length = prev->Keys[prev->nKeys-1].Length;
                            }
                        else
                            {
                            for (j=searchNext->nKeys; j>=0; j--)
                                {
                                searchNext->Children[j+1].Child = searchNext->Children[j].Child;
                                }
                            searchNext->Children[0].Child = prev->Children[prev->nKeys].Child;
                            pBPNodeKey k = bpt_i_FindReplacementKey(searchNext->Children[1].Child, prev->Keys[prev->nKeys-1].Value, prev->Keys[prev->nKeys-1].Length);
                            searchNext->Keys[0].Value = k->Value;
                            searchNext->Keys[0].Length = k->Length;
                            }

                        searchNext->nKeys++;
                        prev->nKeys--;
                        
                        // replace key in this
                        this->Keys[i].Value = prev->Keys[prev->nKeys].Value;
                        this->Keys[i].Length = prev->Keys[prev->nKeys].Length;
                        }
                    else if (next && next->nKeys >= T_SLOTS)
                        {
                        // move a key/child from next to searchNext
                        searchNext->Keys[searchNext->nKeys].Value = next->Keys[0].Value;
                        searchNext->Keys[searchNext->nKeys].Length = next->Keys[0].Length;
                        searchNext->nKeys++;
                        
                        int j;
                        for (j=1; j<next->nKeys; j++)
                            {
                            searchNext->Keys[j-1].Value = searchNext->Keys[j].Value;
                            searchNext->Keys[j-1].Length = searchNext->Keys[j].Length;
                            }
                        if (searchNext->IsLeaf)
                            {
                            for (j=0; j<next->nKeys; j++)
                                {
                                next->Children[j-1].Child = next->Children[j].Child;
                                }
                            searchNext->Children[searchNext->nKeys].Ref = next->Children[0].Ref;
                            }
                        else
                            {
                            for (j=1; j<=next->nKeys; j++)
                                {
                                next->Children[j-1].Child = next->Children[j].Child;
                                }
                            searchNext->Children[searchNext->nKeys+1].Child = prev->Children[0].Child;
                            }
                        
                        next->nKeys--;
                        
                        // replace key in this
                        this->Keys[i].Value = newKey->Value;
                        this->Keys[i].Length = newKey->Length;
                        this->Keys[i+1].Value = next->Keys[0].Value;
                        this->Keys[i+1].Length = next->Keys[0].Length;
                        }
                    else
                        {
                        int j;
                        if (!searchNext->IsLeaf)
                            {
                            pBPNodeKey k = bpt_i_FindReplacementKey(searchNext->Children[0].Child, prev->Keys[prev->nKeys-1].Value, prev->Keys[prev->nKeys-1].Length);
                            prev->Keys[prev->nKeys].Value = k->Value;
                            prev->Keys[prev->nKeys].Length = k->Length;
                            }
                        for (j=0; j<searchNext->nKeys; j++) 
                            {
                            prev->Keys[prev->nKeys+j+(searchNext->IsLeaf ? 0 : 1)].Value = searchNext->Keys[j].Value;
                            prev->Keys[prev->nKeys+j+(searchNext->IsLeaf ? 0 : 1)].Length = searchNext->Keys[j].Length;
                            }
                        for (j=0; j<=searchNext->nKeys; j++) 
                            {
                            prev->Children[prev->nKeys+j+(searchNext->IsLeaf ? 0 : 1)].Child = searchNext->Children[j].Child;
                            }
                        prev->nKeys += searchNext->nKeys + (searchNext->IsLeaf ? 0 : 1);
                        for (j=i; j<this->nKeys-1; j++)
                            {
                            this->Keys[j].Value = this->Keys[j+1].Value;
                            this->Keys[j].Length = this->Keys[j+1].Length;
                            }
                        for (j=i+1; j<this->nKeys; j++)
                            {
                            this->Children[j].Child = this->Children[j+1].Child;
                            }
                        this->nKeys--;
                        nmFree(searchNext, sizeof(BPNode));
                        searchNext = prev;
                        }
                    }

                // if this->nKeys is now 0, replace this with searchNext
                if (this->nKeys == 0)
                    {
                    if (parent == NULL) tree->root = searchNext;
                    else parent->Children[thisIndex].Child = searchNext;
                    nmFree(this, sizeof(BPNode));
                    }
                parent = this;
                thisIndex = i;
                this = searchNext;  // recursively delete key from searchNext

                // TODO consolidate this code with the code for not found in this node
                }
            }
        else
            {
            if (this->IsLeaf)
                {
                return -1;  // not found (this should never happen)
                }
            else
                {
                int nNIndex = i;
                pBPNode searchNext = this->Children[nNIndex].Child;   // note both Prev,Next may not exist but at least 1 does
                pBPNode prev = (nNIndex <= 0 ? NULL : this->Children[nNIndex-1].Child);
                pBPNode next = (nNIndex >= this->nKeys ? NULL : this->Children[nNIndex+1].Child);
                if (searchNext->nKeys < T_SLOTS)
                    {
                    if (nNIndex > 0 && prev->nKeys >= T_SLOTS)
                        {
                        // shift searchNext->Keys and ->Children up by 1
                        int j;
                        for (j=searchNext->nKeys; j>0; j--)
                            {
                            searchNext->Keys[j].Value = searchNext->Keys[j-1].Value;
                            searchNext->Keys[j].Length = searchNext->Keys[j-1].Length;
                            }
                        for (j=searchNext->nKeys+1; j>0; j--)
                            {
                            searchNext->Children[j].Child = searchNext->Children[j-1].Child;
                            }
                        searchNext->nKeys++;

                        // recap:
                        // if searchNext is leaf, set 0th key of searchNext to rightmost key of prev
                        // else, set 0th key of searchNext to key of this
                        // set key of this to rightmost key of prev
                        // set 0th child of searchNext to rightmost child of prev
                        // decrement prev->nKeys

                        // add to searchNext
                        if (searchNext->IsLeaf)
                            {
                            searchNext->Keys[0].Value = prev->Keys[prev->nKeys-1].Value;
                            searchNext->Keys[0].Length = prev->Keys[prev->nKeys-1].Length;
                            }
                        else
                            {
                            searchNext->Keys[0].Value = this->Keys[nNIndex-1].Value;
                            searchNext->Keys[0].Length = this->Keys[nNIndex-1].Length;
                            }
                        searchNext->Children[0].Child = prev->Children[prev->nKeys-(searchNext->IsLeaf ? 1 : 0)].Child;
                        
                        // move key to this
                        this->Keys[nNIndex-1].Value = prev->Keys[prev->nKeys-1].Value;
                        this->Keys[nNIndex-1].Length = prev->Keys[prev->nKeys-1].Length;

                        prev->nKeys--;
                        }
                    else if (nNIndex < this->nKeys && next->nKeys >= T_SLOTS)
                        {
                        // recap:
                        // if searchNext is leaf, set key of this to 1th key of next
                        // else, set key of this to 0th key of next 
                        // set searchNext->nKeysth key of searchNext to key of this
                        // set searchNext->nKeysth child of searchNext to 0th child of next
                        
                        // add to searchNext
                        searchNext->Keys[searchNext->nKeys].Value = this->Keys[nNIndex].Value;
                        searchNext->Keys[searchNext->nKeys].Length = this->Keys[nNIndex].Length;
                        searchNext->Children[searchNext->nKeys+(searchNext->IsLeaf ? 0 : 1)].Child = next->Children[0].Child;
                        searchNext->nKeys++;
                        
                        // move key to this
                        this->Keys[nNIndex].Value = next->Keys[(searchNext->IsLeaf ? 1 : 0)].Value;
                        this->Keys[nNIndex].Length = next->Keys[(searchNext->IsLeaf ? 1 : 0)].Length;

                        // shift next->Keys and ->Children down by 1
                        int j;
                        for (j=1; j<next->nKeys; j++)
                            {
                            next->Keys[j-1].Value = next->Keys[j].Value;
                            next->Keys[j-1].Length = next->Keys[j].Length;
                            }
                        for (j=1; j<=searchNext->nKeys+2; j++)
                            {
                            next->Children[j-1].Child = next->Children[j].Child;
                            }
                        next->nKeys--;
                        }
                    else
                        {
                        // merge
                        pBPNode mergeThis;
                       
                        if (nNIndex > 0)    // merge predecessor, searchNext
                            {
                            mergeThis = searchNext;
                            searchNext = prev;
                            nNIndex--;
                            }
                        else                // merge searchNext, successor
                            {
                            mergeThis = next;
                            }

                        int j;
                        if (searchNext->IsLeaf)
                            {
                            for (j=0; j<mergeThis->nKeys; j++)
                                {
                                searchNext->Keys[searchNext->nKeys+j].Value = mergeThis->Keys[j].Value;
                                searchNext->Keys[searchNext->nKeys+j].Length = mergeThis->Keys[j].Length;
                                }
                            for (j=0; j<=mergeThis->nKeys; j++)
                                {
                                searchNext->Children[searchNext->nKeys+j].Child = mergeThis->Children[j].Child;
                                }
                            searchNext->nKeys += mergeThis->nKeys;
                            }
                        else 
                            {
                            searchNext->Keys[searchNext->nKeys].Value = this->Keys[nNIndex].Value;
                            searchNext->Keys[searchNext->nKeys].Length = this->Keys[nNIndex].Length;
                            for (j=0; j<mergeThis->nKeys; j++)
                                {
                                searchNext->Keys[searchNext->nKeys+j+1].Value = mergeThis->Keys[j].Value;
                                searchNext->Keys[searchNext->nKeys+j+1].Length = mergeThis->Keys[j].Length;
                                }
                            for (j=0; j<=mergeThis->nKeys; j++)
                                {
                                searchNext->Children[searchNext->nKeys+j+1].Child = mergeThis->Children[j].Child;
                                }
                            searchNext->nKeys += mergeThis->nKeys + 1;
                            }
                        // remove key and mergeThis from this
                        for (j=nNIndex; j<this->nKeys-1; j++)
                            {
                            this->Keys[j].Value = this->Keys[j+1].Value;
                            this->Keys[j].Length = this->Keys[j+1].Length;
                            }
                        for (j=nNIndex+1; j<this->nKeys; j++)
                            {
                            this->Children[j].Child = this->Children[j+1].Child;
                            }
                        this->nKeys--;
                        nmFree(mergeThis, sizeof(BPNode));

                        // if this->nKeys is now 0, replace this with searchNext
                        if (this->nKeys == 0)
                            {
                            if (parent == NULL) tree->root = searchNext;
                            else parent->Children[thisIndex].Child = searchNext;
                            nmFree(this, sizeof(BPNode));
                            }
                        }
                    }
                parent = this;
                thisIndex = nNIndex;    // TODO can replace nNIndex with i?
                this = searchNext;
                }
            }
        }
    }

pBPNodeKey bpt_i_FindReplacementKey(pBPNode this, char* key, int key_len)
    {
    while (1)
        {
        if (this->IsLeaf)
            {
            int i = 0;
            while (i < this->nKeys && bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) >= 0) i++;
            return &(this->Keys[i]);
            }
        else
            {
            int i = 0;
            while (i < this->nKeys && bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) >= 0) i++;
            assert (i < this->nKeys+1);
            this = this->Children[i].Child;
            assert (this->nKeys > 0);
            }
        }
    }


/*** bptInit() - initialize an already-allocated node **/
int
bptInit(pBPNode this)
    {

    /** Should not be passed NULL **/
    assert(this != NULL);
    memset(this, 0, sizeof(BPNode));
    /** Clear out the data structure **/
    this->Next = this->Prev = NULL;
    this->nKeys = 0;
    this->IsLeaf = 1;

    return 0;
    }

/*** bptFree() - deinit and deallocate a node and all its descendants
 ***/
void
bptFree(pBPNode this)
    {
    bptDeInit(this);
    nmFree(this, sizeof(BPNode));
    return;
    }

/*** bptDeInit() - deinit a node and deinit/deallocate all descendants, but don't deallocate it
 ***/
int
bptDeInit(pBPNode this)
    {

    /** Should not be passed NULL **/
    assert(this != NULL);

    int i;

    /** Deallocate children **/
    if (!this->IsLeaf)
        {
        for (i = 0; i < this->nKeys; i++)
            {
            bptFree(this->Children[i].Child);
            }
        }

    /** Deallocate key values **/
    for (i = 0; i < this->nKeys; i++)
        {
        nmSysFree(this->Keys[i].Value);
        }
    this->nKeys = 0;

    this->Next = this->Prev = NULL;

    return 0;
    }

/*** bpt_i_Compare() - compares two key values.  Return value is greater
 *** than 0 if key1 > key2, less than zero if key1 < key2, and equal to
 *** zero if key1 == key2.
 ***/
int
bpt_i_Compare(char *key1, int key1_len, char *key2, int key2_len)
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

/** bptSearch(k) - returns (node_y, i) where node_y.keys[i] = k if the key is found; else returns NULL ***/
pBPNode bptSearch(pBPNode this, char* key, int key_len) // formerly bptLookup
    {
    int i = 0;
    int cmp;
    if (this->IsLeaf) 
        {
        while (i < this->nKeys)
            {
            cmp = bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length);
            if (cmp < 0) return NULL;
            else if (cmp == 0) return this; // should be (this, i)
            i++;
            }
        return NULL;
        }
    else 
        {
        while (i < this->nKeys && bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) >= 0) i++;
        return bptSearch(this->Children[i].Child, key, key_len);
        }
    }


/*** bpt_i_Clear() - Frees all keys of this node; if this is a leaf, frees all data values;
 *** if not leaf, calls bptClear on children and frees child nodes.
 *** Calls the provided free_fn for each data value, as:  free_fn(free_arg, value)
 ***/
// TODO unused
int
bpt_i_Clear(pBPNode this, int (*free_fn)(), void *free_arg)
    {
    int i;

    /** Clear child subtrees first */
    if (this->IsLeaf)
        {
        for (i = 0; i < this->nKeys; i++) free_fn(free_arg, this->Children[i].Ref);
        }
    else
        {
        for (i = 0; i <= this->nKeys; i++)
            {
            bpt_i_Clear(this->Children[i].Child, free_fn, free_arg);
            nmFree(this->Children[i].Child, sizeof(BPNode));
            }
        }
    
    /** Clear key/value pairs */
    for (i = 0; i < this->nKeys; i++)
        {
        nmSysFree(this->Keys[i].Value);
        }

    return 0;   // TODO handle nmFree, nmSysFree return nonzero
    }


void
bpt_PrintTree(pBPNode tree, int level)
    {
    int i;
    for (i=0; i<level; i++) printf("\t");
    printf("Node ");
    printPtr(tree);
    printf(": %sleaf, children: ", (tree->IsLeaf ? "" : "non"));
    if (tree->IsLeaf)
        {
        for (i=0; i<tree->nKeys; i++) printf("(%s, %s) ", tree->Keys[i].Value, (char*)tree->Children[i].Ref);
        printf("\n");
        }
    else
        {
        for (i=0; i<=tree->nKeys; i++) 
            {
            printPtr(tree->Children[i].Child);
            if (i < tree->nKeys) printf("%s ", tree->Keys[i].Value);
            }
        printf("\n");
        for (i=0; i<=tree->nKeys; i++) bpt_PrintTree(tree->Children[i].Child, level+1);
        }
    if (level==0) printf("----------------------\n");
    }

void printPtr(void* ptr) {
    // change 8 hex digits = 32 bits into a "name" (divide into six 6-bit vals 62-127 incl )
    int tmp = (int)ptr;
    int i;
    for (i=0; i<3; i++) {
        int tmp2 = (tmp & 0x3f)+65;
        if (tmp2 > 90) tmp2 += 6;
        if (tmp2 > 122) tmp2 -= 75;
        printf("%c", tmp2);
        tmp = tmp >> 6;
    }
    printf(" ");
}

int testTree(BPTree* tree)
    {
    int last = -1;
    int lastLeaf = -1;
    if (tree->root->nKeys == 0) return 0;
    return testTree_inner(tree->root, &last, &lastLeaf);
    }
int testTree_inner(pBPNode tree, int* last, int* lastLeaf)
    {
    int curr, i;
    if (tree->IsLeaf)
        {
        if (tree->nKeys == 0) 
            {
            printf("Failed on empty leaf node\n");
            return -1;
            }
        for (i=0; i<tree->nKeys; i++) 
            {
            curr = atoi(tree->Keys[i].Value);
            if (((*last) >= 0 && (*last) != curr) || (*lastLeaf) >= curr || curr != atoi(tree->Children[i].Ref+3)) 
                {
                if ((*last) >= 0 && (*last) != curr) printf("Failed on first leaf %d not same as index %d\n", curr, (*last));
                else if ((*lastLeaf)==curr) printf("Failed on dup leaf: %d\n", curr);
                else printf("Failed on nonmatching val: %d %s\n", curr, (char*)(tree->Children[i].Ref));
                return -1;
                }
            (*last) = -1;
            (*lastLeaf) = curr;
            }
        }
    else
        {
        for (i=0; i<=tree->nKeys; i++) 
            {
            if (testTree_inner(tree->Children[i].Child, last, lastLeaf) < 0) return -1;
            if (i < tree->nKeys)
                {
                curr = atoi(tree->Keys[i].Value); 
                if ((*last) > curr) { printf("Failed on %d > %d\n", (*last), curr); return -1; }
                (*last) = curr;
                }
            }
        }

	return 0;
    }
