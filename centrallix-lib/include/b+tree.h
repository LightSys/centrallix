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

#define BPT_SLOTS	(16)
#define CEIL_HALF_OF_LEAF_SLOTS ( ( BPT_SLOTS + 1 ) / 2 )
#define IDX_SLOTS ( ( BPT_SLOTS ) - 1 )
#define CEIL_HALF_OF_IDX_SLOTS	( ( ( IDX_SLOTS ) + 1 ) / 2 )

/** Structures **/
typedef struct _BPK BPTreeKey, *pBPTreeKey;
typedef union _BPV BPTreeVal, *pBPTreeVal;
typedef struct _BPT BPTree, *pBPTree;

struct _BPK
    {
    char*	Value;
    size_t	Length;
    };

union _BPV
    {
    BPTree*	Child;
    void*	Ref;
    };

struct _BPT
    {
    BPTreeKey	Keys[BPT_SLOTS];
    int		nKeys;
    BPTreeVal	Children[BPT_SLOTS+1];
    pBPTree	Parent;
    pBPTree	Next;
    pBPTree	Prev;
    unsigned int		IsLeaf:1;
    };

/** Functions **/
pBPTree bptNew();
int bptInit(pBPTree this);
void bptFree(pBPTree this);
int bptDeInit(pBPTree this);
int bptAdd(pBPTree this, char* key, int key_len, void* data);
void* bptLookup(pBPTree this, char* key, int key_len);
int bptRemove(pBPTree this, char* key, int key_len);
int bptClear(pBPTree this, int (*free_fn)(), void* free_arg);
int bpt_i_Push(pBPTree this);
int bpt_i_Fake();
int bpt_PrintTree(pBPTree root);
void bpt_i_CopyKey(pBPTree tree1, int idx1, pBPTree tree2, int idx2);
int bpt_i_GetNeighborIndex(pBPTree this);
pBPTree bpt_i_RemoveEntryFromNode(pBPTree this, char* key, int key_len, pBPTreeVal);
pBPTree bpt_i_AdjustRoot(pBPTree this);
pBPTree bpt_i_DeleteEntry(pBPTree root, pBPTree this, char* key, int key_len, pBPTreeVal);
int bpt_i_Compare(char* key1, int key1_len, char* key2, int key2_len);
int bpt_i_Scan(pBPTree this, char* key, int key_len, int *locate_index);
int bpt_i_Find(pBPTree this, char* key, int key_len, pBPTree locate, int *locate_index);

#endif /* _BPTREE_H */

