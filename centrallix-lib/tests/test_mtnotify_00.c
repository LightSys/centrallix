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
/* Module: 	test_mtnotify_00.c     					*/
/* Author:	Micah Shennum 					        */
/* Creation:	Jul 22, 2011 					        */
/* Description: Test send and wait                                  */
/************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "mtask.h"
#include "mtnotify.h"
#include "xstring.h"
#include "xarray.h"

#define maxIters 500

void wait_fn(void *data) {
    int i;
    pEvent mess;
    XArray pinglist;
    xaInit(&pinglist,1);
    xaAddItem(&pinglist,"ping");
    for(i=0; i<maxIters; i++){
        mess = mtnWaitForEvents(&pinglist,1,16);
        printf("Got: %s %s\n",mess->type,(char*)mess->param);
        mtnDeleteEvent(mess);
    }//end for
    thExit();
    return;
}

void send_fn(void *data){
    int i;
    pEvent ping;
    for(i=0; i<maxIters; i++){
        ping=mtnCreateEvent("ping",data,i,0);
        mtnSendEvent(ping);
        thSleep(200);
    }//end for
    thExit();
    return;
}

long long
test(char **tname) {
    int iters;
    *tname = "mtnotify-00 send and wait for event";
    mtnInitialize();
    thCreate(wait_fn,0,(void *)" Hello world");
    thCreate(send_fn,0,(void *)" Hello world");
    for(iters=0;iters<maxIters;iters++)thSleep(700);
    mtnDeinitialize();
    return 2*maxIters;
}//end test
