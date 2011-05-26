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
/* Module: 	test_util_00.c     					*/
/* Author:	Micah Shennum 					        */
/* Creation:	May 26th, 2011 					        */
/* Description: Test strtoi                                             */
/************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "util.h"

#define TXT_SIZE 1024
#define RANGE    500000
long long
test(char** tname){
    int i;
    char text[TXT_SIZE]="";
    *tname = "util-00 convertion from strings to integers";
    
    for(i=-RANGE; i<RANGE; i++){
        snprintf(text,TXT_SIZE,"%d",i);
        assert(strtoi(text,NULL,0)==i);
    }//end for all range
    
    //long too big for int
    snprintf(text,TXT_SIZE,"%ld",INT_MAX+7L);
    assert(strtoi(text,NULL,0)==INT_MAX);
    snprintf(text,TXT_SIZE,"%ld",INT_MIN-7L);
    assert(strtoi(text,NULL,0)==INT_MIN);
    
    //too big for long?
    assert(strtoi("121340193481047193741092347298347291391741",NULL,0)==INT_MAX);
    assert(strtoi("-19874238479349128374239483948347834913498",NULL,0)==INT_MIN);
    
    //return what all we did
    return 2*RANGE+4;
}//end test