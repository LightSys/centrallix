#ifndef XTREE_H
#define	XTREE_H

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
/* Module: 	xtree.c, xtree.h  					*/
/* Author:	Daniel Rothfus  					*/
/* Creation:	July 26, 2011   					*/
/* Description:	Tree based associative table for centrallix-lib.  This  */
/* 		enables one to find the closest match if so desired.    */
/************************************************************************/

typedef struct _XTND{
    char* key, *data;
    struct _XTND* less, *greater;
} XTreeNode,    *pXTreeNode;

typedef struct{
    pXTreeNode  root;
    int		KeyLen;
    int		nItems;
} XTree, *pXTree;

int xtInit(pXTree tree, int keyLength);
int xtDeInit(pXTree tree);
int xtAdd(pXTree this, char* key, char* data);
int xtRemove(pXTree this, char* key);
char* xtLookup(pXTree this, char* key);
int xtClear(pXTree this, int (*free_fn)(), void* free_arg);
char* xtLookupBeginning(pXTree this, char* key);

#endif	/* XTREE_H */

