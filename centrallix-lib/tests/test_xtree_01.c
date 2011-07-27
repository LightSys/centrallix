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
#include "xarray.h"
#include "xhash.h"

#define TESTSCOUNT    5000
long long
test(char** tname){
    int i;
    char *name;
    char *data;
    pXTree tree;
    pXHashTable datalist;
    *tname = "xtree-01 adding and fetching from xtree";

    tree=nmMalloc(sizeof(XTree));
    datalist=nmMalloc(sizeof(XHashTable));

    xtInit(tree,0);
    xhInit(datalist,500,0);
    
    for(i=0;i<TESTSCOUNT;i++){
        name=nmMalloc(50);
        data=nmMalloc(50);
        snprintf(name,50,"key%x",i);
        snprintf(data,50,"%x%x",rand()%500,rand()%500);
        xtAdd(tree,name,data);
        xhAdd(datalist,name,data);
    }

    for(i=0;i<TESTSCOUNT;i++){
        char *dataH,*dataT;
        name=nmMalloc(50);
        snprintf(name,50,"key%x",i);
        dataT=xtLookup(tree,name);
        dataH=xhLookup(datalist,name);
        assert(!strncmp(dataH,dataT,50));
    }
    
    xhDeInit(datalist);
    xtDeInit(tree);
    //two loops with two ops each
    return 2*(TESTSCOUNT*2);
}//end test
