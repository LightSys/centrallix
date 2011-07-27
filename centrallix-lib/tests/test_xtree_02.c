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

long long
test(char** tname){
    int i;
    char *key;
    pXTree tree;
    *tname = "xtree-02 adding and fetching with xtLookupBeginning";
    alarm(0);
    tree=nmMalloc(sizeof(XTree));

    assert(!xtInit(tree,0));
    
    i=-1;
    while(filedata[++i][0]!=0)
        assert(!xtAdd(tree,filedata[i][1],filedata[i][0]));

    i=-1;
    while(filedata[++i][0]!=0)
        assert(!strcmp(xtLookupBeginning(tree,filedata[i][1]),filedata[i][0]));

    i=-1;
    while(filedata[++i][0]!=0){
        key=malloc(strlen(filedata[i][1])+4);
        snprintf(key,strlen(filedata[i][1])+4,"%s%x",strlen(filedata[i][1])+8,rand()%0xff);
        assert(!strcmp(xtLookupBeginning(tree,key),filedata[i][0]));
        free(key);
    }

    i=-1;
    while(filedata[++i][0]!=0)
        assert(!xtRemove(tree,filedata[i][1]));

    assert(!xtDeInit(tree));
    alarm(5);
    sleep(2);
    //three loops with one ops each
    return 3*(TESTSCOUNT);
}//end test
