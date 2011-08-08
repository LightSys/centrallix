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

#define TESTSCOUNT    500
long long
test(char** tname){
    int i,j;
    pXTree tree;
    *tname = "xtree-00 adding and removing from xtree";
    tree=nmMalloc(sizeof(XTree));

    assert(xtInit(tree,'/') == 0);
    
    //try removing non-existant data
    assert(xtRemove(tree,"Han Solo")<0);
    assert(xtRemove(tree,"Yoda")<0);
    assert(xtRemove(tree,"Darth Maul")<0);
    assert(xtRemove(tree,"Luke")<0);

    //assert(tree->nItems==0);
    //finally, a stress test
    for(j=0;j<TESTSCOUNT;j++){
        i=-1;
        while(filedata[++i][0]!=0)
            assert(!xtAdd(tree,filedata[i][1],filedata[i][0]));

        //assert(tree->nItems==i);

        i=-1;
        while(filedata[++i][0]!=0){
            int val = xtRemove(tree,filedata[i][1]);
            assert(!val);
        }
    }//end j
    assert(xtDeInit(tree) == 0);
    return 2*TESTSCOUNT;
}//end test
