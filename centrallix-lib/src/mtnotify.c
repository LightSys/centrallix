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

volatile int mtn_internal_running;
pXArray mtn_internal_currentlyWaiting; // List of semaphores waiting for events
pEvent mtn_interal_currentEvent;

int mtnInitialize(){
    mtn_internal_running = 1;
    mtn_interal_currentEvent = NULL;
    mtn_internal_currentlyWaiting = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(mtn_internal_currentlyWaiting, 16);
    return 0;
}

void mtnDeinitialize(){
    int numSemaphores;
    
    mtn_internal_running = 0;

    // Clear all contents and drop all semaphores
    numSemaphores = xaCount(mtn_internal_currentlyWaiting);
    while(numSemaphores--){
        syDestroySem(xaGetItem(mtn_internal_currentlyWaiting, numSemaphores), 0);
    }
    
    xaDeInit(mtn_internal_currentlyWaiting);
    nmFree(mtn_internal_currentlyWaiting, sizeof(XArray));
}

pEvent mtnCreateEvent(pXString typeOfEvent, void * data, int priority, int flags){
  // Allocate structure
  pEvent event = (pEvent)nmMalloc(sizeof(Event));
  // Allocate string
  event->type = (char *)nmMalloc(typeOfEvent->Length + 1);
  memcpy(event->type, typeOfEvent->String, typeOfEvent->Length + 1);
  event->priority = priority;
  event->flags = flags;
  event->param = data;
  event->refcount = 0;
  return event;
}

void mtnDeleteEvent(pEvent event){
  if(--event->refcount > 0)return;
  nmFree(event->type,strlen(event->type) + 1);
  nmFree(event,sizeof(Event));
  return;
}

int mtnSendEvent(pEvent event){
    
    // Set current event
    mtn_interal_currentEvent = event;
    mtn_interal_currentEvent->refcount += xaCount(mtn_internal_currentlyWaiting);
    
    // Awake all semaphores
    numSemaphores = xaCount(mtn_internal_currentlyWaiting);
    while(numSemaphores--){
        xaGetItem(mtn_internal_currentlyWaiting, numSemaphores);
    }
}

pEvent mtnWaitForEvents(pXArray eventStrings, int blocking, int prioity){
    pSemaphore createdSemaphore;
    int numEventTypes, numSemaphores;
    pEvent foundItem;

    // Create semaphore and add into array
    if(blocking){
        createdSemaphore = syCreateSem(1, 0);
        xaAddItem(mtn_internal_currentlyWaiting, createdSemaphore);
    }

    // While has not found an event
    while(blocking){

        // Wait for semaphore to break
        if(blocking){
            syPostSem(pSemaphore, 1, 0);
        }

        // Iterate through all needed event types
        numEventTypes = xaCount(eventStrings);
        while(numEventTypes--){

            // If is not running, return NULL
            if(mtn_internal_running == 0){
                return NULL;
            }

            // If qualifies, remove semaphore from array and return the event
            if(strcmp(((pXString)xaGetItem(eventStrings, numEventTypes))->String, mtn_interal_currentEvent->type) == 0 && mtn_interal_currentEvent->priority <= prioity){

                // Remove this thread's semaphore from list and destroy semaphore
                if(blocking){
                    
                    numSemaphores = xaCount(currentSemaphores);
                    while(numSemaphores--){
                        if(xaGetItem(currentSemaphores, numSemaphores) == createdSemaphore){
                            xaRemoveItem(currentSemaphores, numSemaphores);
                        }
                    }
                    syDestroySem(createdSemaphore);
                }

                // Return needed event
                return mtn_interal_currentEvent;
            }   
        }

        // If not, increment semaphore again and start up
        mtnDeleteEvent(mtn_interal_currentEvent);
        if(blocking){
            syGetSem(pSemaphore, 1, 0);
        }
    }    
}
