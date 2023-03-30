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
    int i, j;
    char text[TXT_SIZE]="";
    *tname = "util-02 check number of bytes in UTF-8 characters";
         
    for(i=0; i<RANGE; i++){
        for(j = 0 ; j <= 0x7F ; j++)
            {
            assert(numBytesInChar(j) == 1);
            }
        for(j = 0x80 ; j <= 0xBF ; j++)
            {
            assert(numBytesInChar(j) == -1);
            }
        for(j = 0xC0 ; j <= 0xDF ; j++)
            {
            assert(numBytesInChar(j) == 2);
            }
        for(j = 0xE0; j <= 0xEF ; j++)
            {
            assert(numBytesInChar(j) == 3);
            }
        for(j = 0xF0 ; j <= 0xF7 ; j++)
            {
            assert(numBytesInChar(j) == 4);
            }
        for(j = 0xF8 ; j <= 0xFF ; j++)
            {
            assert(numBytesInChar(j) == -1);
            }
    }
    
    
    return RANGE;
}//end test
