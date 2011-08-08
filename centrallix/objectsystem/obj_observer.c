#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "obj.h"
#include <cxlib/mtask.h>
#include <cxlib/newmalloc.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	obj_observer.c    					*/
/* Author:	Daniel Rothfus (GRB)					*/
/* Creation:	April 29, 1999  					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.     */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		==> obj_observer.c Collects a lot of the common         */
/*              functionality of the observer system in the Centrallix  */
/*              system.                                       		*/
/************************************************************************/

#ifndef __OBJ_OBSERVER_INTERNALS__
#define __OBJ_OBSERVER_INTERNALS__

/** \brief This initializes the object observer contents of an objectsystem
 session.
 \return If the initialization was successful.
 */
int obj_internal_ObserverInitSession(pObjSession sess){
    return xhInit(&sess->OpenObservers, 4, sizeof(pObjObserver));
}

/** \brief This is the free function to pass to the XHashTable to free items
 */
int obj_internal_HashFreeFunction(pObjObserver obs){
    objCloseObserver(obs);
    return 0;
}

/** \brief Clear out the observer related contents of an objectsystem session.
 */
void obj_internal_ObserverDeInitSession(pObjSession sess){
    
    // Clear out observers from the list
    xhClear(&sess->OpenObservers, (int (*)())obj_internal_HashFreeFunction, NULL);
    
    // Delete the list
    xhDeInit(&sess->OpenObservers);
}

inline void obj_internal_PushHeadEvent(pGlobalObserver globalObserver){
    pObjObserverEventNode newNode;
    mssError(0,"OBS","Pushing head event at %s", globalObserver->Pathname);
    newNode = nmMalloc(sizeof(ObjObserverEventNode));
    if(!newNode){
        fprintf(stderr, "Could not allocate new event node!  Lossy observers coming up!\n");
        return;
    }
    newNode->Next = globalObserver->HeadEvent;
    newNode->SpecificPathname = NULL;
    newNode->NumObservers = globalObserver->NumObservers;
    globalObserver->HeadEvent = newNode;
}

/** \brief Free an event node.  Note that this does not free any nodes after
 the event pointed to by next, because it does not know if it should.
 */
inline void obj_internal_FreeEventNode(pObjObserverEventNode node){
    if(node){
        nmFree(node, sizeof(ObjObserverEventNode));
    }
}

/** \brief This frees the global observer specified and tries to release it 
 from the global tree of global observers.
 */
inline void obj_internal_FreeGlobalObserver(pGlobalObserver globalObserver){
    if(globalObserver){
        xtRemove(&OSYS.ObservedObjects, globalObserver->Pathname); // Remove it from the tree if it is there

        // Free if possible
        if(globalObserver->Pathname){
            nmFree(globalObserver->Pathname, strlen(globalObserver->Pathname) + 1);
            obj_internal_FreeEventNode(globalObserver->HeadEvent);
        }
        nmFree(globalObserver, sizeof(GlobalObserver));
    }
}

/** \brief This function is made for checking if an update of any type to an object
 needs to be registered with any object observers. 
 */
inline void obj_internal_ObserverCheckObservers(char* path, ObjObserverEventType type){
    pGlobalObserver globalObserver;

    mssError(0,"OBS","Found event at path %s", path);
    
    // Lookup in global to see if there is something with this path
    if((globalObserver = (pGlobalObserver)xtLookupBeginning(&OSYS.ObservedObjects, path))){
        mssError(0,"OBS","Event triggers %s", globalObserver->Pathname);
        
        // Add event parameters
        globalObserver->HeadEvent->SpecificPathname = path; // Null terminate path
        globalObserver->HeadEvent->EventType = type;
        globalObserver->HeadEvent->NumObservers = globalObserver->NumObservers;

        // Push new event
        obj_internal_PushHeadEvent(globalObserver);
        
    }
    
}

#endif

pObjObserver objOpenObserver(pObjSession objSess, char* path){
    char *tmp;
    mssError(0, "OBS", "Observer created at %s", path);
    pObjObserver toReturn = NULL;
    pGlobalObserver globalObserver = NULL;
    size_t pathLen;
    
    // Allocate and initialize observer object
    toReturn = nmMalloc(sizeof(ObjObserver));
    if(!toReturn){
        goto initialization_error;
    }
    pathLen = strlen(path);
    pathLen = ((path[pathLen - 2] == '/') ? pathLen - 1 : pathLen); // Get rid of trailing slash
    toReturn->Pathname = nmSysMalloc(pathLen);
    if(!toReturn->Pathname){
        goto initialization_error;
    }
    strncpy(toReturn->Pathname, path, pathLen);
    toReturn->Pathname[pathLen - 1] = '\0'; // Make sure the path is null terminated
    //if there is a *, eat upto the previous /
    tmp = strchr(toReturn->Pathname,'*');
    if(tmp){
        for(;*tmp!='/';tmp--);
        *tmp = 0;
    }
    toReturn->RegisteredSession = objSess;   
    toReturn->DeleteAbilityState = OBJ_OBSERVER_OK_DELETE;
    
    // Add link to global observer object creating if necessary
    globalObserver = (pGlobalObserver)xtLookup(&OSYS.ObservedObjects, toReturn->Pathname);
    if(!globalObserver){
        
        // Create new global observer for this object
        globalObserver = nmMalloc(sizeof(GlobalObserver));
        if(!globalObserver){
            goto initialization_error;
        }
        
        globalObserver->NumObservers = 0;
        globalObserver->Pathname = nmMalloc(pathLen);
        if(!globalObserver->Pathname){
            goto initialization_error;
        }
        strcpy(globalObserver->Pathname, toReturn->Pathname);
        
        // Initialize head event
        obj_internal_PushHeadEvent(globalObserver);
        if(!globalObserver->HeadEvent){
            goto initialization_error;
        }
        xtAdd(&OSYS.ObservedObjects, globalObserver->Pathname, (char *)globalObserver);
    }
    
    // Reference count!
    globalObserver->NumObservers++;
    globalObserver->HeadEvent++;
    
    // Fill in the rest of the fields
    toReturn->GlobalObserver = globalObserver;
    toReturn->CurrentEvent = globalObserver->HeadEvent;
    
    return toReturn;
    
initialization_error: // Error handling!  Bad mallocs stop here!
    if(toReturn){
        if(toReturn->Pathname){
            nmFree(toReturn->Pathname, strlen(toReturn->Pathname) + 1);
        }
        nmFree(toReturn, sizeof(ObjObserver));
    }
    obj_internal_FreeGlobalObserver(globalObserver);
    return NULL;
}

int objCloseObserver(pObjObserver obs){
    mssError(0, "OBS", "Observer closed at %s", obs->Pathname);
    // Remove from objectsystem session if necessary
    if(obs->DeleteAbilityState != OBJ_OBSERVER_DO_DELETE_IN_POLL){
        xhRemove(&obs->RegisteredSession->OpenObservers, obs->Pathname);
    }
    
    // See if it is in a poll
    if(obs->DeleteAbilityState == OBJ_OBSERVER_NO_DELETE_POLLING){
        obs->DeleteAbilityState = OBJ_OBSERVER_DO_DELETE_IN_POLL;
    }
    
    // If it is one of the two final deletion states, delete it
    if(obs->DeleteAbilityState != OBJ_OBSERVER_NO_DELETE_POLLING){
        // Walk up stack of events to free them if necessary
        while(objPollObserver(obs, 0, NULL) != OBJ_OBSERVER_EVENT_NONE);

        // See if it is necessary to clear out the global
        if(--obs->GlobalObserver->NumObservers == 0){
            obj_internal_FreeGlobalObserver(obs->GlobalObserver);
        }
        
        nmSysFree(obs->Pathname);
    }
    
    return 0;
}

ObjObserverEventType objPollObserver(pObjObserver obs, int blocking, char** specificPath){
    ObjObserverEventType toReturn = OBJ_OBSERVER_EVENT_NONE;
    *specificPath = NULL;
    //mssError(0, "OBS", "Observer polled at %s", obs->Pathname);
    while(1){
        if(obs->GlobalObserver->HeadEvent != obs->CurrentEvent){
            if(specificPath){
                *specificPath = obs->CurrentEvent->SpecificPathname;
            }
            toReturn = obs->CurrentEvent->EventType;
            break;
        }
        if(obs->DeleteAbilityState == OBJ_OBSERVER_DO_DELETE_IN_POLL){
            objCloseObserver(obs);
            break;
        }
        if(blocking){
            thYield();
        }
        else{
            break;
        }
    }
    
    // Check to delete the returned event
    if(obs->GlobalObserver->HeadEvent != obs->CurrentEvent && obs->CurrentEvent->NumObservers-- == 0){
        obj_internal_FreeEventNode(obs->CurrentEvent);
    }
    
    return toReturn;
}

ObjObserverEventType objPollObservers(pXArray obss, int blocking, pObjObserver* updated, char** specificPath){
    ObjObserverEventType toReturn = OBJ_OBSERVER_EVENT_NONE;
    pObjObserver currentObserver;
    int arrayLen = xaCount(obss);
    int c = arrayLen;
    
    while(1){
        while(c--){
            currentObserver = (pObjObserver)xaGetItem(obss, c);
            if(currentObserver->DeleteAbilityState == OBJ_OBSERVER_DO_DELETE_IN_POLL){ // Catch one that should be deleted
                xaRemoveItem(obss, c);
                arrayLen--;
                objPollObserver(currentObserver, 0, specificPath);
                continue;
            }
            toReturn = objPollObserver(currentObserver, 0, specificPath); // Poll the observer
            if(toReturn != OBJ_OBSERVER_EVENT_NONE){
                break;
            }
        }
        if(!blocking || arrayLen == 0){
            break;
        }
        else{
            c = arrayLen;
            thYield();
        }
    }
    
    return toReturn;
}
