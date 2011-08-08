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
#include "xhash.h"

#include "test_xtree_filedata.h"

#define TESTSCOUNT    500

long long
test(char** tname){
    int i,j;
    pXTree tree;
    pXHashTable datalist;
    *tname = "xtree-01 adding and fetching from xtree";

    tree=nmMalloc(sizeof(XTree));
    datalist=nmMalloc(sizeof(XHashTable));

    assert(!xtInit(tree,'/'));
    xhInit(datalist,TESTSCOUNT,0);
    
    assert(!xtAdd(tree,"NullA",NULL));
    assert(!xtAdd(tree,"NullB",NULL));
    assert(!xtAdd(tree,"NullC",NULL));
    
    assert(xtLookup(tree,"NullA")==NULL);
    assert(xtLookup(tree,"NullB")==NULL);
    assert(xtLookup(tree,"NullC")==NULL);

    assert(!xtRemove(tree,"NullA"));
    assert(!xtRemove(tree,"NullB"));
    assert(!xtRemove(tree,"NullC"));

    //finally, a stress test
    for(j=0;j<TESTSCOUNT;j++){
        i=-1;
        while(filedata[++i][0]!=0)
            assert(!xtAdd(tree,filedata[i][1],filedata[i][0]));

        i=-1;
        while(filedata[++i][0]!=0)
            assert(!strcmp(xtLookup(tree,filedata[i][1]),filedata[i][0]));

        i=-1;
        while(filedata[++i][0]!=0)
            assert(!xtRemove(tree,filedata[i][1]));
    }
    xhDeInit(datalist);
    assert(!xtDeInit(tree));
    //two loops with two ops each
    return 2*(TESTSCOUNT*2);
}//end test
