#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mtask.h"

#define THREADS  3
#define RUNCOUNT 500

pSemaphore alldone;

void threadA(void *data){
    int i;
    syGetSem(alldone,1,0);
    thSetName(thCurrent(),"ThreadA");
    for(i=0;i<RUNCOUNT;i++){
        thSleep(10);
    }
    syPostSem(alldone,1,0);
    return;
}

void threadB(void *data){
    int i=5;
    syGetSem(alldone,1,0);
    thSetName(thCurrent(),"ThreadB");
    //4 is the officaly chosen random number
    srand(4);
    while(i<RUNCOUNT){
        i*=rand()%i;
        thYield();
    }
    syPostSem(alldone,1,0);
}

void threadC(void *data){
    int i,j,k;
    syGetSem(alldone,1,0);
    thSetName(thCurrent(),"ThreadC");
    //n^3 :)
    for(i=0;i<RUNCOUNT;i++){
        for(j=0;j<RUNCOUNT;j++){
            for(k=0;k<RUNCOUNT;k++);
        }
    }
    syPostSem(alldone,1,0);
}

long long test(char** tname){
    *tname = "mtask 00, a fake test of threading";
    alldone=syCreateSem(THREADS,0);
    thCreate(threadA,0,NULL);
    thCreate(threadB,0,NULL);
    thCreate(threadC,0,NULL);
    thSleep(20);
    syDestroySem(alldone,0);
    return RUNCOUNT*THREADS;
}//end test