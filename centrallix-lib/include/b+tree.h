#ifndef _BPTREE_H
#define _BPTREE_H

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

/* DO NOT SET HALF_T_SLOTS to < 3 */
#define HALF_T_SLOTS (8)     
#define MAX_KEYS(pNode) (2 * HALF_T_SLOTS - ((pNode)->IsLeaf ? 0 : 1))

/** Structures **/
typedef struct _BPK BPNodeKey, *pBPNodeKey;
typedef union _BPV BPNodeVal, *pBPNodeVal;
typedef struct _BPN BPNode, *pBPNode;
typedef struct BPTree BPTree, *pBPTree;
typedef struct _BPI BPIter, *pBPIter;

//IMT: Temporary until we know data structure that will be passed to bulk load
/*typedef struct BPTData
    {
    char * Key;
    int    KeyLength;
    void * Value;
    } BPTData;*/

struct BPTree {
	pBPNode root;
    int size;
};

struct _BPK
    {
    char*	Value;
    size_t	Length;
    };

union _BPV
    {
    BPNode*	Child;
    void*	Ref;
    };

struct _BPN
    {
    BPNodeKey	Keys[2 * HALF_T_SLOTS];  // last key left unused for index nodes
    int		nKeys;
    BPNodeVal	Children[2 * HALF_T_SLOTS];
    pBPNode	Next;
    pBPNode	Prev;
    unsigned int		IsLeaf:1;
    };

struct _BPI
    {
    void* Ref;

    pBPNode Curr;
    int Index;
    unsigned int Direction:1;   //0 is forward, 1 is reverse
    };

/** Public Functions **/
pBPTree bptNew();
int bptInit(pBPTree this);
int bptAdd(pBPTree this, char* key, int key_len, void* data);
int bptRemove(pBPTree this, char* key, int key_len, int (*free_fn)(), void* free_arg);
void* bptLookup(pBPTree this, char* key, int key_len);
pBPIter bptFront(pBPTree this);
pBPIter bptBack(pBPTree this);
void bptNext(pBPIter this, int *status);
void bptPrev(pBPIter this, int *status);
int bptIterFree(pBPIter this);
int bptSize(pBPTree this);
int bptIsEmpty(pBPTree this);
int bptFree(pBPTree this, int (*free_fn)(), void* free_arg);
int bptDeInit(pBPTree this, int (*free_fn)(), void* free_arg);

/** Internal Functions **/
pBPNode bpt_i_new_BPNode();                                                 // allocate and init a new BPNode, return NULL if fails
int bpt_i_Compare(char* key1, int key1_len, char* key2, int key2_len);      // helper for several internal functions
int bpt_i_Split_Child(pBPNode this, int index);                             // helper for bptRemove
int bpt_i_Insert_Nonfull(pBPNode this, char* key, int key_len, void* data); // helper for bptRemove
pBPNodeKey bpt_i_FindReplacementKey(pBPNode this, char* key, int key_len);  // helper for bptRemove
void* bpt_I_Lookup(pBPNode this, char* key, int key_len);                   // helper for bptLookup
int bpt_i_Clear(pBPNode this, int (*free_fn)(), void* free_arg);            // used in bptFree and bptDeInit
int bptInit_I_Node(pBPNode this);                                           // helper for bpt_i_new_BPNode

// functions from orig file that haven't been rechecked/implemented
//void bpt_i_ReplaceValue(pBPNode this, char* find, int find_len, char* replace, int replace_len);
pBPTree bptBulkLoad(char* fname, int num);

/** Temporary functions (should not exist in final version) **/
void printTree2(pBPTree tree, int level);
void printTree(pBPNode tree, int level);
void printPtr(void* ptr);   // helper function for "naming" nodes
int testTree(pBPTree tree);   // test that all nodes are in order, no dups except for in index nodes, vals match keys
int testTree_inner(pBPNode tree, int* last, int* lastLeaf);    // helper for testTree

#endif /* _BPTREE_H */

