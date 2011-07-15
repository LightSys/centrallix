#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cxlib/mtask.h>
#include "mtask.h"


//from mtask.c, update to match
#define MAX_THREADS		256

#define THREADS MAX_THREADS*2

pSemaphore alldone;

void threadRun(void *data){
    thSleep(10);
    syPostSem(alldone,1,0);
    return;
}

long long test(char** tname){
    int i;
    *tname = "mtask 01, creates threads";
    //atexit(thFreeUnusedStacks);
    alldone=syCreateSem(MAX_THREADS-4,0);
    for(i=0;i<THREADS;i++){
        syGetSem(alldone,1,0);
        thCreate(threadRun,0,(void *)i);
    }
    syDestroySem(alldone,0);
    return THREADS*4;
}