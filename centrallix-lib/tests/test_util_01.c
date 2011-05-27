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
    *tname = "util-01 convertion from strings to unsigned integers";
         
    for(i=-RANGE; i<RANGE; i++){
        snprintf(text,TXT_SIZE,"%d",i);
        assert(strtoui(text,NULL,0)==(unsigned int)i);
    }//end for all range
    
    //long too big for uint
    snprintf(text,TXT_SIZE,"%ld",UINT_MAX+7L);
    assert(strtoui(text,NULL,0)==UINT_MAX);
    
    //too big for long?
    assert(strtoui("121340193481047193741092347298347291391741",NULL,0)==UINT_MAX);
    
    //return what all we did
    return 2*RANGE+2;
}//end test