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
/* Module: 	test_xtree_00.c     					*/
/* Author:	Micah Shennum 					        */
/* Creation:	Jul 27, 2011 					        */
/* Description: Test add remove                                         */
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

long long hit_count;

void freethingy(void *data, long long *hits){
    assert(data);
    (*hits)++;
    return;
}

long long
test(char** tname){
    int i,test;
    pXTree tree;
    *tname = "xtree-03 adding and clearing xtree";
    tree=nmMalloc(sizeof(XTree));

    assert(xtInit(tree,'/') == 0);

    for(test=0;test<TESTSCOUNT;test++){
        i=-1;
        while(filedata[++i][0]!=0)
            assert(!xtAdd(tree,filedata[i][1],filedata[i][0]));

        assert(!xtClear(tree,NULL,NULL));
        //assert(tree->nItems==0);
        assert(!xtClear(tree,NULL,NULL));
        assert(!xtClear(tree,NULL,NULL));
    }

    i=-1;
    while(filedata[++i][0]!=0)
            assert(!xtAdd(tree,filedata[i][1],filedata[i][0]));

    hit_count = 0;
    assert(!xtClear(tree,freethingy,&hit_count));
    assert(hit_count == i);

    assert(xtDeInit(tree) == 0);
    return 2*TESTSCOUNT;
}//end test
