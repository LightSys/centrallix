/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2005 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	test_xtree_04.c     					*/
/* Author:	Micah Shennum 					        */
/* Creation:	Aug 01, 2011 					        */
/* Description: Test iterating over the tree                            */
/************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "xtree.h"
#include "newmalloc.h"

#include "test_xtree_filedata.h"

#define TESTSCOUNT    5000

char *last;

void freethingy(void *data, void *hits){
    assert(data);
    return;
}

void treeiter(char *key, char *data, void *userData){
    if(last)assert(strcmp(key,last)>0);
    assert(!strcmp(data,xtLookup(userData,key)));
    last=key;
    return;
}

long long
test(char** tname){
    int i,test;
    pXTree tree;
    *tname = "xtree-04 iterating over xtree";
    tree=nmMalloc(sizeof(XTree));

    assert(xtInit(tree,0) == 0);

    for(test=0;test<TESTSCOUNT/8;test++){
    i=-1;
    while(filedata[++i][0]!=0)
            assert(!xtAdd(tree,filedata[i][1],filedata[i][0]));
    last=NULL;
    xtTraverse(tree,treeiter,tree);
    assert(!xtClear(tree,freethingy,NULL));
    assert(xtDeInit(tree) == 0);

    }

    return 2*TESTSCOUNT;
}//end test
