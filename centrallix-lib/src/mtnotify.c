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
volatile pXArray mtn_internal_currentlyWaiting; // List of semaphores waiting for events
volatile pEvent mtn_internal_currentEvent;
volatile pSemaphore mtn_internal_sendEventSemaphore;

int mtnInitialize(){
    mtn_internal_running = 1;
    mtn_internal_currentEvent = NULL;
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

pEvent mtnCreateEvent(char *typeOfEvent, void * data, int priority, int flags){
  // Allocate structure
  pEvent event = (pEvent)nmMalloc(sizeof(Event));
  // Allocate string
  event->type = strdup(typeOfEvent);
  event->priority = priority;
  event->flags = flags;
  event->param = data;
  event->refcount = 0;
  return event;
}

void mtnDeleteEvent(pEvent event){
  if(--event->refcount > 0)return;
  free(event->type);
  nmFree(event,sizeof(Event));
  return;
}

int mtnSendEvent(pEvent event){
    int numSemaphores;
    
    // Set current event
    mtn_internal_currentEvent = event;
    mtn_internal_currentEvent->refcount += xaCount(mtn_internal_currentlyWaiting);
    
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
    do{

        // Wait for semaphore to break
        if(blocking){
            syPostSem(createdSemaphore, 1, 0);
        }
        
        // Make sure that there is an event to look at
        if(mtn_internal_currentEvent){

            // Iterate through all needed event types
            numEventTypes = xaCount(eventStrings);
            while(numEventTypes--){

                // If is not running, return NULL
                if(mtn_internal_running == 0){
                    return NULL;
                }

                // If qualifies, remove semaphore from array and return the event
                if(strcmp((char *)xaGetItem(eventStrings, numEventTypes), mtn_internal_currentEvent->type) == 0 && mtn_internal_currentEvent->priority <= prioity){

                    // Remove this thread's semaphore from list and destroy semaphore
                    if(blocking){

                        numSemaphores = xaCount(mtn_internal_currentlyWaiting);
                        while(numSemaphores--){
                            if(xaGetItem(mtn_internal_currentlyWaiting, numSemaphores) == createdSemaphore){
                                xaRemoveItem(mtn_internal_currentlyWaiting, numSemaphores);
                            }
                        }
                        syDestroySem(createdSemaphore, 0);
                    }

                    // Return needed event
                    return mtn_internal_currentEvent;
                }  // End if to check if event types are same
            } // End while(numEventTypes--) 
            
            // If there was an event, take place in getting rid of our part of it.
            mtnDeleteEvent(mtn_internal_currentEvent);
        }// End if to check if there is an event

        // If not, increment semaphore again and start up
        if(blocking){
            syGetSem(createdSemaphore, 1, 0);
        }
    }while(blocking);
}//end mtnWaitForEvents
