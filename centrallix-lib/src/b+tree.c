// TODO figure out what we want bptSearch to return (other than NULL for not found)
// TODO set prev and next pointers
//I am Zac!


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

/*** Return pointer to newly allocated and initialized BPNode, or NULL if fails ***/
pBPNode 
bpt_i_new_BPNode()
    {
    pBPNode newNode;

    newNode = (pBPNode)nmMalloc(sizeof(BPNode));
    if (!newNode) return NULL;

    if (bptInit_I_Node(newNode) != 0)
        {
        nmFree(newNode, sizeof(pBPNode));
        return NULL;
        }

    return newNode;
    }

/*** Given nonfull internal node this and index such that this.Children[index] is full,
 *   split this.Children[index] and adjust this to have an additional child
 ***/
int
bpt_i_Split_Child(pBPNode this, int index)
    {
    pBPNode oldChild, newChild;

    assert (this->nKeys < MAX_KEYS(this));

    oldChild = this->Children[index].Child;
    assert (oldChild->nKeys == MAX_KEYS(oldChild));

    newChild = bpt_i_new_BPNode();
    if (!newChild) return -1;

    newChild->IsLeaf = oldChild->IsLeaf;

    /*** If had n keys in original node, only need n-1 keys with two nodes ***/
    newChild->nKeys = HALF_T_SLOTS - (oldChild->IsLeaf ? 0 : 1);
    oldChild->nKeys = HALF_T_SLOTS - (oldChild->IsLeaf ? 0 : 1);

    if (oldChild->IsLeaf) 
        {
        memmove(newChild->Keys, &oldChild->Keys[newChild->nKeys], newChild->nKeys * sizeof(newChild->Keys[0]));
        memmove(newChild->Children, &oldChild->Children[oldChild->nKeys], newChild->nKeys * sizeof(newChild->Children[0]));
        newChild->Next = oldChild->Next;
        newChild->Prev = oldChild;
        oldChild->Next = newChild;
        }
    else 
        {
        memmove(newChild->Keys, &oldChild->Keys[newChild->nKeys+1], newChild->nKeys * sizeof(newChild->Keys[0]));
        memmove(newChild->Children, &oldChild->Children[oldChild->nKeys+1], newChild->nKeys * sizeof(newChild->Children[0]));
        newChild->Children[newChild->nKeys].Child = oldChild->Children[newChild->nKeys+oldChild->nKeys+1].Child;
        }

    memmove(&this->Children[index+2], &this->Children[index+1], (this->nKeys-index) * sizeof(this->Children[0]));
    this->Children[index+1].Child = newChild;

    memmove(&this->Keys[index+1], &this->Keys[index], (this->nKeys-index+1) * sizeof(this->Keys[0]));
    this->Keys[index] = oldChild->Keys[HALF_T_SLOTS-(oldChild->IsLeaf ? 0 : 1)];

    this->nKeys++;
    return 0;
    }

/*** Inserts key, data into node "this", assumed to be nonfull ***/
int
bpt_i_Insert_Nonfull(pBPNode this, char* key, int key_len, void* data)
    {
    int i;

    while (1)
        {
        assert (this->nKeys < MAX_KEYS(this));

        i = this->nKeys-1;
        if (this->IsLeaf) 
            {
            while (i >= 0 && bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) < 0) 
                {
                this->Keys[i+1].Value = this->Keys[i].Value;
                this->Keys[i+1].Length = this->Keys[i].Length;
                this->Children[i+1].Ref = this->Children[i].Ref;
                i--;
                }

            this->Keys[i+1].Value = nmMalloc(sizeof(BPNodeVal));
            if (!this->Keys[i+1].Value) return -1;

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
            if (this->Children[i].Child->nKeys == MAX_KEYS(this->Children[i].Child))
                {
                if (bpt_i_Split_Child(this, i) < 0) return -2;
                if (bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) > 0) i++;
                }
            
            this = this->Children[i].Child;
            }
        }
    }
/*** Creates a pointer to a new B+ Tree and creates its root node. No initialization occurs ***/
pBPTree
bptNew()
    {
    pBPTree this;

    this = nmMalloc(sizeof(BPTree));
    if (!this) return NULL;

    this->root = bpt_i_new_BPNode();
    if (!this->root) return NULL;

    this->size = 0;

    return this;
    }

/*** bptAdd(T, k, v) - insert k,v into tree T in a single pass down the tree ***/
int
bptAdd(pBPTree this, char* key, int key_len, void* data)
    {
    pBPNode newRoot;
    if(this == NULL || key == NULL || data == NULL) {
        return -1;
    }

    if (bptLookup(this, key, key_len) != NULL)
        {
        /*** Key already exists; duplicate keys can't be inserted into a B+ Tree***/
        /*** without breaking the search structure                              ***/

        //TODO the behavior for dup-insert-attempt should be defined somewhere;
        //If multiple values need to be stored, then might need a linked list
        return -1;  
        }
    
    if (this->root->nKeys == MAX_KEYS(this->root))
        {
        /*** Full ***/
        newRoot = bpt_i_new_BPNode();
        if (!newRoot) return -1;

        newRoot->IsLeaf = 0;
        newRoot->Children[0].Child = this->root;
        this->root = newRoot;

        if (bpt_i_Split_Child(newRoot, 0) < 0) return -2;
        }

    if (bpt_i_Insert_Nonfull(this->root, key, key_len, data) < 0) return -3;

    this->size++;
    return 0;
    }

/***    bpt_i_Find_Key_In_Node(node, key, key_len, cmp)
 * returns the index i that key is at in node and updates cmp to the result of the comparison.
 * ***/
int
bpt_i_Find_Key_In_Node(BPNode * node, char * key, int key_len, int * cmp)
{
    int i, curr_cmp;

    curr_cmp = 1;
    for (i = 0; i < node->nKeys; i++)
        {
        curr_cmp = bpt_i_Compare(key, key_len, node->Keys[i].Value, node->Keys[i].Length);
        if (curr_cmp <= 0) break;
        }

    if(cmp)
        {
        *cmp = curr_cmp;
        }
    return i;
}

/*
bptRemove(T, K, L, free(), D)
T: Tree that you will be removing a specific node from
K: Key of the node that has the data you are hoping to remove
L: Length of K
free(): Some free function
D: a pointer for the data; can be any data type

For int (*free_fn), we understand it is difficult to know what to pass in there. Here's an example of a simple dummy free function:
int bpt_dummy_freeFn(void* arg, void* ptr) {
    free(ptr);
    return 0;
}

*/
int
bptRemove(pBPTree tree, char* key, int key_len, int (*free_fn)(), void* free_arg)
    {
    pBPNode this, parent, searchNext, prev, next, mergeThis;
    pBPNodeKey newKey, k;
    int i, j, thisIndex, nNIndex, cmp;

    if (bptLookup(tree, key, key_len) == NULL) return -1;

    this = tree->root;
    parent = NULL;

    while (1)
        {
        i = bpt_i_Find_Key_In_Node(this, key, key_len, &cmp);
        if (cmp == 0)   
            {
            if (this->IsLeaf)
                {
                nmFree(this->Keys[i].Value, this->Keys[i].Length);
                free_fn(free_arg, this->Children[i].Ref);
                memmove(&this->Keys[i], &this->Keys[i+1], ((this->nKeys-1)-i) * sizeof(this->Keys[0]));
                memmove(&this->Children[i], &this->Children[i+1], ((this->nKeys-1)-i) * sizeof(this->Children[0]));
                this->nKeys--;

                tree->size--;
                return 0;
                }
            else
                {
                searchNext = this->Children[i+1].Child;
                newKey = bpt_i_FindReplacementKey(this, key, key_len);
                if (searchNext->nKeys >= HALF_T_SLOTS)     
                    {
                    this->Keys[i].Value = newKey->Value;
                    this->Keys[i].Length = newKey->Length;
                    }
                else
                    {
                    prev = this->Children[i].Child;
                    next = (i+2 <= this->nKeys ? this->Children[i].Child : NULL);
                    
                    if (prev->nKeys >= HALF_T_SLOTS)                    
                        {
                        /* move a key/child from prev to searchNext */
                        memmove(&searchNext->Keys[1], &searchNext->Keys[0], sizeof(searchNext->Keys[0]) * searchNext->nKeys);

                        if (searchNext->IsLeaf)
                            {
                            memmove(&searchNext->Children[1], &searchNext->Children[0], sizeof(searchNext->Children[0]) * searchNext->nKeys);
                            searchNext->Children[0].Ref = prev->Children[prev->nKeys-1].Ref;
                            searchNext->Keys[0].Value = prev->Keys[prev->nKeys-1].Value;
                            searchNext->Keys[0].Length = prev->Keys[prev->nKeys-1].Length;
                            }
                        else
                            {
                            memmove(&searchNext->Children[1], &searchNext->Children[0], sizeof(searchNext->Children[0]) * (searchNext->nKeys + 1));
                            searchNext->Children[0].Child = prev->Children[prev->nKeys].Child;
                            k = bpt_i_FindReplacementKey(searchNext->Children[1].Child, prev->Keys[prev->nKeys-1].Value, prev->Keys[prev->nKeys-1].Length);
                            searchNext->Keys[0].Value = k->Value;
                            searchNext->Keys[0].Length = k->Length;
                            }

                        searchNext->nKeys++;
                        prev->nKeys--;
                        
                        /* replace key in this node */
                        this->Keys[i].Value = prev->Keys[prev->nKeys].Value;
                        this->Keys[i].Length = prev->Keys[prev->nKeys].Length;
                        }
                    else if (next && next->nKeys >= HALF_T_SLOTS)
                        {
                        /* move a key/child from next to searchNext */
                        searchNext->Keys[searchNext->nKeys].Value = next->Keys[0].Value;
                        searchNext->Keys[searchNext->nKeys].Length = next->Keys[0].Length;
                        searchNext->nKeys++;
                        
                        memmove(&searchNext->Keys[0], &searchNext->Keys[1], sizeof(searchNext->Keys[0]) * (next->nKeys-1));
                        if (searchNext->IsLeaf)
                            {
                            memmove(&searchNext->Children[0], &searchNext->Children[1], sizeof(searchNext->Children[0]) * next->nKeys);
                            searchNext->Children[searchNext->nKeys].Ref = next->Children[0].Ref;
                            }
                        else
                            {
                            memmove(&next->Children[0], &searchNext->Children[1], sizeof(searchNext->Children[0]) * next->nKeys);
                            searchNext->Children[searchNext->nKeys+1].Child = prev->Children[0].Child;
                            }
                        
                        next->nKeys--;
                        
                        /* replace key in this node */
                        this->Keys[i].Value = newKey->Value;
                        this->Keys[i].Length = newKey->Length;
                        this->Keys[i+1].Value = next->Keys[0].Value;
                        this->Keys[i+1].Length = next->Keys[0].Length;
                        }
                    else
                        {
                        if (!searchNext->IsLeaf)
                            {
                            k = bpt_i_FindReplacementKey(searchNext->Children[0].Child, prev->Keys[prev->nKeys-1].Value, prev->Keys[prev->nKeys-1].Length);
                            prev->Keys[prev->nKeys] = *k;
                            }
                        for (j=0; j<searchNext->nKeys; j++) 
                            {
                            prev->Keys[prev->nKeys+j+(searchNext->IsLeaf ? 0 : 1)] = searchNext->Keys[j];
                            }
                        for (j=0; j<=searchNext->nKeys; j++) 
                            {
                            prev->Children[prev->nKeys+j+(searchNext->IsLeaf ? 0 : 1)].Child = searchNext->Children[j].Child;
                            }
                        prev->nKeys += searchNext->nKeys + (searchNext->IsLeaf ? 0 : 1);
                        for (j=i; j<this->nKeys-1; j++)
                            {
                            this->Keys[j] = this->Keys[j+1];
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

                /* if this->nKeys is now 0, replace this with searchNext */
                if (this->nKeys == 0)
                    {
                    if (parent == NULL)
                        {
                        tree->root = searchNext;
                        }
                    else
                        {
                        parent->Children[thisIndex].Child = searchNext;
                        }
                    nmFree(this, sizeof(BPNode));
                    }
                parent = this;
                thisIndex = i;
                this = searchNext; 
                /* recursively delete key from searchNext */
                // TODO consolidate this code with the code for not found in this node
                }
            }
        else
            {
            assert(this->IsLeaf);
                {
                nNIndex = i;
                searchNext = this->Children[nNIndex].Child;   
                prev = (nNIndex <= 0 ? NULL : this->Children[nNIndex-1].Child);
                next = (nNIndex >= this->nKeys ? NULL : this->Children[nNIndex+1].Child);
                if (searchNext->nKeys < HALF_T_SLOTS)
                    {
                    if (nNIndex > 0 && prev->nKeys >= HALF_T_SLOTS)
                        {
                        memmove(&searchNext->Keys[1], &searchNext->Keys[0], sizeof(searchNext->Keys[0]) * searchNext->nKeys);
                        memmove(&searchNext->Children[1], &searchNext->Children[0], sizeof(searchNext->Children[0]) * (searchNext->nKeys + 1));
                        searchNext->nKeys++;

                        /* recap:
                        * if searchNext is leaf, set 0th key of searchNext to rightmost key of prev
                        * else, set 0th key of searchNext to key of this
                        * set key of this to rightmost key of prev
                        * set 0th child of searchNext to rightmost child of prev
                        * decrement prev->nKeys
                        */

                        /* add to searchNext */
                        if (searchNext->IsLeaf)
                            {
                            searchNext->Keys[0] = prev->Keys[prev->nKeys-1];
                            }
                        else
                            {
                            searchNext->Keys[0] = this->Keys[nNIndex-1];
                            }
                        searchNext->Children[0].Child = prev->Children[prev->nKeys-(searchNext->IsLeaf ? 1 : 0)].Child;
                        
                        this->Keys[nNIndex-1] = prev->Keys[prev->nKeys-1];

                        prev->nKeys--;
                        }
                    else if (nNIndex < this->nKeys && next->nKeys >= HALF_T_SLOTS)
                        {
                        /* recap:
                        * if searchNext is leaf, set key of this to 1th key of next
                        * else, set key of this to 0th key of next 
                        * set searchNext->nKeysth key of searchNext to key of this
                        * set searchNext->nKeysth child of searchNext to 0th child of next
                        */
                        
                        /* add to searchNext */
                        searchNext->Keys[searchNext->nKeys] = this->Keys[nNIndex];
                        searchNext->Children[searchNext->nKeys+(searchNext->IsLeaf ? 0 : 1)].Child = next->Children[0].Child;
                        searchNext->nKeys++;
                        
                        /* move key to this */
                        this->Keys[nNIndex] = next->Keys[(searchNext->IsLeaf ? 1 : 0)];

                        /* shift next->Keys and ->Children down by 1 */
                        memmove(&next->Keys[0], &next->Keys[1], sizeof(next->Keys[0]) * (next->nKeys-1));
                        memmove(&next->Children[0], &next->Children[1], sizeof(next->Children[0]) * (next->nKeys+1));
                        next->nKeys--;
                        }
                    else
                        {
                        /* merge the nodes */
                        if (nNIndex > 0)   
                            {
                            /* merge predecessor, searchNext */
                            mergeThis = searchNext;
                            searchNext = prev;
                            nNIndex--;
                            }
                        else                
                            {
                            /* merge searchNext, successor */
                            mergeThis = next;
                            }

                        if (searchNext->IsLeaf)
                            {
                            memmove(&searchNext->Keys[searchNext->nKeys], mergeThis->Keys, sizeof(mergeThis->Keys[0]) * mergeThis->nKeys);
                            memmove(&searchNext->Children[searchNext->nKeys], mergeThis->Children, sizeof(mergeThis->Children[0]) * (mergeThis->nKeys+1));
                            searchNext->nKeys += mergeThis->nKeys;
                            //fix next and prev pointers
                            searchNext->Next = mergeThis->Next;
                            mergeThis->Next->Prev=searchNext;
                            }
                        else 
                            {
                            searchNext->Keys[searchNext->nKeys] = this->Keys[nNIndex];
                            memmove(&searchNext->Keys[searchNext->nKeys + 1], mergeThis->Keys, sizeof(mergeThis->Keys[0]) * mergeThis->nKeys);
                            memmove(&searchNext->Children[searchNext->nKeys + 1], mergeThis->Children, sizeof(mergeThis->Children[0]) * (mergeThis->nKeys+1));
                            searchNext->nKeys += mergeThis->nKeys + 1;
                            }

                        /* remove key and mergeThis from this */
                        memmove(&this->Keys[nNIndex], &this->Keys[nNIndex+1], sizeof(this->Keys[0]) * ((this->nKeys-1)-nNIndex));
                        memmove(&this->Children[nNIndex+1], &this->Children[nNIndex+2], sizeof(this->Children[0]) * (this->nKeys-(nNIndex+1)));
                        this->nKeys--;
                        nmFree(mergeThis, sizeof(BPNode));

                        /* if this->nKeys is now 0, replace this with searchNext */
                        if (this->nKeys == 0)
                            {
                            if (parent == NULL)
                                {
                                tree->root = searchNext;
                                //For adding sibs in
                                tree->root->Next = NULL;
                                tree->root->Prev = NULL;
                                }
                            else
                                {
                                //For adding sibs in
                                searchNext->Prev = this->Prev;  //TODO: Could this->Prev be replaced with NULL?
                                searchNext->Next = this->Next;  //      Could this->Next be replaced with NULL?
                                parent->Children[thisIndex].Child = searchNext;
                                }
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

/*** returns the size of the tree ***/
int
bptSize(pBPTree this)
    {
    return this->size;
    }

/*** returns zero iff the tree is empty ***/
int
bptIsEmpty(pBPTree this)
    {
    return (this->size == 0);
    }

/*** helper function to find a key to replace when repairing the tree structure when removing ***/
pBPNodeKey
bpt_i_FindReplacementKey(pBPNode this, char* key, int key_len)
    {
    int i;
    while (1)
        {
        i = 0;
        while (i < this->nKeys && bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) >= 0) i++;
        if (this->IsLeaf)
            {
            return &(this->Keys[i]);
            }
        else
            {
            assert (i < this->nKeys+1);
            this = this->Children[i].Child;
            assert (this->nKeys > 0);
            }
        }
    }


/*** bptInit() - initialize an already-allocated tree **/
int
bptInit(pBPTree this)
    {
    /** Should not be passed NULL **/
    assert(this != NULL);
    /** Clear out the data structure **/
    memset(this, 0, sizeof(BPTree));
    this->root = bpt_i_new_BPNode();
    if (!this->root) return NULL;
    this->size = 0;
    return 0;
    }

/*** bptInit_I_Node() - initialize an already-allocated node **/
int
bptInit_I_Node(pBPNode this)
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

/*** bptDeInit() - deinit a tree and deinit/deallocate all nodes, but don't deallocate it.
 *** Calls the provided free_fn for each data value, as:  free_fn(free_arg, value)
 ***/
int
bptDeInit(pBPTree this, int (*free_fn)(), void* free_arg)
    {
    int i, ret;
    /** Should not be passed NULL **/
    if(this==NULL) return -1;
    pBPNode root = this->root;

    /** Deallocate children **/
    ret = bpt_i_Clear(root, free_fn, free_arg);
    printf("%d\n", ret);
    if(ret != 0) return -2;

    this->size = 0;
    
    return 0;
    }

/*** bptFree() - deinit and deallocate a tree and all its nodes
 *** Calls the provided free_fn for each data value, as:  free_fn(free_arg, value)
 ***/
int
bptFree(pBPTree this, int (*free_fn)(), void* free_arg)
    {
    int ret;
    pBPNode root = this->root;

    ret = bpt_i_Clear(root, free_fn, free_arg);
    if(ret != 0) return -1;
        
    nmFree(this, sizeof(BPTree));
    
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
    len = (key1_len > key2_len) ? key2_len : key1_len;

    /** Compare **/
    rval = memcmp(key1, key2, len);

    /** Initial parts same: compare based on lengths of keys. **/
    if (rval == 0)
        {
        rval = key1_len - key2_len;
        }

    return rval;
    }

/*** bptLookup(tree, k, k_len) - returns the value stored for the key ***/
void* bptLookup(pBPTree this, char* key, int key_len) 
    {
    int i, cmp;
    pBPNode root = this->root;


    if (root->IsLeaf) 
        {
        i = bpt_i_Find_Key_In_Node(root, key, key_len, &cmp);
        return ((cmp == 0) ? root->Children[i].Ref : NULL);
        }
    else 
        {
        i = 0;
        while (i < root->nKeys && bpt_i_Compare(key, key_len, root->Keys[i].Value, root->Keys[i].Length) >= 0) i++;
        return bpt_I_Lookup(root->Children[i].Child, key, key_len);
        }
    }

/*** recursive helper for bptLookup ***/
void* bpt_I_Lookup(pBPNode this, char* key, int key_len) 
    {
    int i, cmp;

    if (this->IsLeaf) 
        {
        i = bpt_i_Find_Key_In_Node(this, key, key_len, &cmp);
        return ((cmp == 0) ? this->Children[i].Ref : NULL);
        }
    else 
        {
        i = 0;
        while (i < this->nKeys && bpt_i_Compare(key, key_len, this->Keys[i].Value, this->Keys[i].Length) >= 0) i++;
        return bpt_I_Lookup(this->Children[i].Child, key, key_len);
        }
    }
/*** Creates a forward iterator for the leaves of the tree starting at the first leaf ***/
pBPIter
bptFront(pBPTree this)
    {
    pBPIter iter = nmMalloc(sizeof(BPIter));
    if(!iter) return NULL;
    
    pBPNode curr = this->root;
    while(!curr->IsLeaf)
        {
        curr = curr->Children[0].Child;
        }
    iter->Curr = curr;
    iter->Direction = 0;
    iter->Index = 0;
    iter->Ref = curr->Children[iter->Index].Ref;
    return iter;
    }

/*** Creates a reverse iterator for the leaves of the tree starting at the last leaf ***/
pBPIter
bptBack(pBPTree this)
    {
    pBPIter iter = nmMalloc(sizeof(BPIter));
    if(!iter) return NULL;
    
    pBPNode curr = this->root;
    while(!curr->IsLeaf)
        {
        curr = curr->Children[curr->nKeys].Child;
        }
    iter->Curr = curr;
    iter->Direction = 1;
    iter->Index = curr->nKeys - 1;
    iter->Ref = curr->Children[iter->Index].Ref;
    return iter;
    }

void
bptNext(pBPIter this, int *status)
    {
    //forward
    if(this->Direction == 0)
        {
        if(this->Index + 1 == this->Curr->nKeys)
            {
            this->Index = 0;
            this->Curr = this->Curr->Next;
            }
        else this->Index++;
        }
    //reverse
    else
        {
        if(this->Index == 0)
            {
            this->Index = this->Curr->Prev->nKeys - 1;
            this->Curr = this->Curr->Prev;
            }
        else this->Index--;
        }

    if(this->Curr == NULL) *status = -1;
    else this->Ref = this->Curr->Children[this->Index].Ref;
    }

void
bptPrev(pBPIter this, int *status)
    {
    //reverse
    if(this->Direction == 1)
        {
        if(this->Index + 1 == this->Curr->nKeys)
            {
            this->Index = 0;
            this->Curr = this->Curr->Next;
            }
        else this->Index++;
        }
    //forward
    else
        {
        if(this->Index == 0)
            {
            this->Index = this->Curr->Prev->nKeys - 1;
            this->Curr = this->Curr->Prev;
            }
        else this->Index--;
        }
    if(this->Curr == NULL) *status = -1;
    else this->Ref = this->Curr->Children[this->Index].Ref;
    }

/*** bpt_i_Clear() - Frees all keys of this node; if this is a leaf, frees all data values;
 *** if not leaf, calls bptClear on children and frees child nodes.
 *** Calls the provided free_fn for each data value, as:  free_fn(free_arg, value)
 ***/
int
bpt_i_Clear(pBPNode this, int (*free_fn)(), void *free_arg)
    {
    int i, ret;
    ret = 0;

    /** Clear child subtrees first */
    if (this->IsLeaf)
        {
        for (i = 0; i < this->nKeys; i++) ret |= free_fn(free_arg, this->Children[i].Ref);
        if(ret != 0)
            {
            return -2;
            }
        }
    else
        {
        for (i = 0; i <= this->nKeys; i++)
            {
            ret |= bpt_i_Clear(this->Children[i].Child, free_fn, free_arg);
            nmFree(this->Children[i].Child, sizeof(BPNode));
            }

        if(ret != 0)
            {
            return -1;
            }
        }
    
    /** Clear key/value pairs */
    for (i = 0; i < this->nKeys; i++)
        {
        nmFree(this->Keys[i].Value, sizeof(BPNodeVal));
        }

    return 0;   // TODO handle nmFree, nmSysFree return nonzero
    }

//TODO : finish function
pBPTree
bptBulkLoad(char* fname, int num)
	{
	pBPTree tree = bptNew();
    int tmp25;
    int i;
	FILE* data = NULL;
	data = fopen(fname, "r");
	char key[10], leaf[50];
	char* info;
	char* key_val;
	
	for (i=0; i<num; i++)
		{
		fscanf(data, "%s %[^\n]", key, leaf);
		printf("Key: %s\nValue: %s\n",key,leaf);
		key_val = key;
        info = leaf;
        tmp25 = bptAdd(tree, key, strlen(key), leaf);
		if (tmp25 != 0)
			{
			return NULL;
			}
		}
	fclose(data);

	return tree;
	}    


/********************** TEST FUNCTIONS *******************************/
void 
printTree2(pBPTree tree, int level) {
    if (tree == NULL) {
        printf("Null");
    }
    pBPNode thisRoot = tree->root;

    int i;
    
    if (thisRoot == NULL) {
        printf("Null");
    }
    else {
        printf("%sRoot Val: ");
        for (i = 0; i < level; i++) {

        }
    }

}

void
printTree(pBPNode tree, int level)
    {
    int i;
    if(tree == NULL) {
        printf("Null");
    }
    else {
    //for (i=0; i<level; i++) {}
    printf("\t");
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
        for (i=0; i<=tree->nKeys; i++) printTree(tree->Children[i].Child, level + 1);
        }
    if (level==0) printf("----------------------\n");
    }
    }

void printPtr(void* ptr) {
    /* change 8 hex digits = 32 bits into a "name" (divide into six 6-bit vals 62-127 incl) */
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

        //For checking sibs
        //for (i=0; i < tree->nKeys; i++) 
        //    {
        //    if( tree->Children[i].Child->Next != tree->Children[i+1].Child )
        //        {
        //        printf("Next pointer invalid.");
        //        return -1;
        //        }

        //    if( tree->Children[i+1].Child->Prev != tree->Children[i].Child )
        //        {
        //        printf("Prev pointer invalid.");
        //        return -1;
        //        }
        //    }
        
        //if( tree->Children[0].Child->Prev != NULL )
        //    {
        //    printf( "First child prev is not NULL.");
        //    return -1;
        //    }

        //if( tree->Children[tree->nKeys].Child->Next != NULL )
        //    {
        //    printf( "Last child next is not NULL.");
        //    return -1;
        //    }
        }

	return 0;
    }
