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

#include "util.h"


long long
test(char** tname){
    int i;
    int iter = 500000;
    *tname = "util-00 convertion from strings to integers";
    
    
     for(i=0;i< iter; i++){
        assert(strtoi("0",NULL,0)==0);
        assert(strtoi("1235",NULL,0)==1235);
        assert(strtoi("0xf0f",NULL,0)==0xf0f);
        assert(strtoi("123154115463544623415345345622315465245351254354345453154532435561254512521435",
                NULL,0)==INT_MAX);
        assert(strtoi("-123154115463544623415345345622315465245351254354345453154532435561254512521435",
                NULL,0)==INT_MIN);
    }
    
    return 5*iter;
}//end test