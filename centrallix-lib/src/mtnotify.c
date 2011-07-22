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
#include "mtask.h"
#include "xstring.h"
#include <stdlib.h>
#include <string.h>

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
    pSemaphore currentSemaphore;
    
    running = 0;

    // Clear all contents and drop all semaphores
    numSemaphores = xaCount(currentlyWaiting);
    while(numSemaphores--){
        currentSemaphore = xaGetItem(currentlyWaiting, numSemaphores);
    }
    
    xaDeInit(currentlyWaiting);
    nmFree(currentlyWaiting, sizeof(XArray));
}

pEvent mtnCreateEvent(pXString typeOfEvent, void * data, int priority, int flags){
  // Allocate structure
  pEvent event = (pEvent)nmMalloc(sizeof(Event));
  // Allocate string
  event->type = (pXString)nmMalloc(sizeof(XString));
  memcmp(event->type, typeOfEvent, sizeof(XString));
  event->priority = priority;
  event->flags = flags;
  event->param = data;
  event->refcount = 0;
  return event;
}

void mtnDeleteEvent(pEvent event){
  if(--event->refcount > 0)return;
  nmFree(event->type,sizeof(XString));
  nmFree(event,sizeof(Event));
  return;
}

int mtnSendEvent(pEvent event){
    
    // Set current event
    currentEvent = event;
    currentEvent->refcount += xaCount(currentlyWaiting);
    
    // Awake all semaphores
    numSemaphores = xaCount(currentlyWaiting);
    while(numSemaphores--){
        xaGetItem(currentlyWaiting, numSemaphores);
    }
}

pEvent mtnWaitForEvents(pXArray eventStrings, int blocking, int prioity ){
    pSemaphore createdSemaphore;
    int arrayLocation, numEventTypes;
    
    // Create semaphore and add into array
    createdSemaphore = sySemCreate(XXX, 0);
    xaAddItem(currentlyWaiting, createdSemaphore);
    
    // While has not found an event
    while(1){
    
        // Wait for semaphore to break
        
    
        // Check if qualifies
        numEventTypes = xaCount(eventStrings);
        while(numEventTypes--){
            
            // If is not running, return NULL
            if(running == 0){
                return NULL;
            }

            // If qualifies, remove semaphore from array and return that
            if(xaGetItem(eventStrings, numEventTypes) == ?? || running = 0){
                    
            }   
        }
        
        // If not, increment semaphore again and start up
    }
}
