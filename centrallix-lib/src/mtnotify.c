/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2011 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      mtnotify.c                                              */
/* Author:      Micah Shennum and Daniel Rothfus                        */
/* Creation:    July 22, 2011                                           */
/* Description: A thread notification system that simplifies the        */
/*              delivery of events to threads that are waiting on them. */
/************************************************************************/

#include "mtnotify.h"
#include "xhash.h"
#include "newmalloc.h"
#include <stdlib.h>

volatile int running;
pXArray currentlyWaiting; // List of semaphores waiting for events
pEvent currentEvent;

void mtn_internal_AddEventToQueue(pEvent event){
    
}

int mtnInitialize(){
    running = 1;
    currentEvent = NULL;
    currentlyWaiting = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(currentlyWaiting, 16);
}

int mtnDeinitialize(){
    int numSemaphores;
    
    running = 0;
    
    // Clear all contents and drop all semaphores
    numSemaphores = xaCount(currentlyWaiting);
    while(numSemaphores--){
        xaGetItem(currentlyWaiting, numSemaphores);
    }
    xaDeInit(currentlyWaiting);
    nmFree(currentlyWaiting, sizeof(XArray));
}

pEvent mtnCreateEvent(pXString typeOfEvent, void * data, int priority, int flags){
    
    // Allocate structure
    
    // Allocate string
    
}

void mtnDeleteEvent(pEvent event){
    
    // Free string
    
    // Free structure
    
}

int mtnSendEvent(pEvent event){
    
    // Add into queue
    
    // Awake all semaphores
    
}

pEvent mtnWaitForEvents(pXArray eventStrings, int blocking, int prioity ){
    
    // Create semaphore and add into array
    
    // While has not found an event
    
        // Wait for semaphore to break
    
        // Check if qualifies
    
            // If is not running, return NULL
    
            // If qualifies, remove semaphore from array and return that
    
            // If not, increment semaphore again
    
}
