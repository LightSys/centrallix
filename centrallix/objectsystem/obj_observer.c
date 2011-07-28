#include <string.h>
#include <stdlib.h>
#include "obj.h"
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
int obj_internal_ObserverInitSession();

/** \brief Clear out the observer related contents of an objectsystem session.
 */
void obj_internal_ObserverDeInitSession();

/** \brief This function is made for checking if an update of any type to an object
 needs to be registered with any object observers. 
 */
inline void obj_internal_ObserverCheckObservers(char* path, ObjObserverEventType type);

inline void obj_internal_PushHeadEvent(pGlobalObserver globalObserver){
    pObjObserverEventNode newNode;
    
    newNode = nmMalloc(sizeof(ObjObserverEventNode));
    newNode->Next = globalObserver->HeadEvent;
    newNode->SpecificPathname = NULL;
    newNode->NumObservers = globalObserver->NumObservers;
    globalObserver->HeadEvent = newNode;
}

#endif

pObjObserver objOpenObserver(pObjSession objSess, char* path){
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
    toReturn->Pathname = nmMalloc(pathLen); 
    if(!toReturn->Pathname){
        goto initialization_error;
    }
    strncpy(toReturn->Pathname, path, pathLen);
    toReturn->Pathname[pathLen - 1] = '\0'; // Make sure the path is null terminated
    
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
    if(globalObserver){
        if(globalObserver->Pathname){
            nmFree(globalObserver->Pathname, strlen(globalObserver->Pathname) + 1);
            if(globalObserver->HeadEvent){
                nmFree(globalObserver->HeadEvent, sizeof(ObjObserverEventNode));
            }
        }
        nmFree(globalObserver, sizeof(GlobalObserver));
    }
    return NULL;
}

int objCloseObserver(pObjObserver obs){
    
    // See if it is necessary to clear out the global
    
    return -1;
}

ObjObserverEventType objPollObserver(pObjObserver obs, int blocking, char** specificPath){
    
    
    // Should we delete this returned event?
    return OBJ_OBSERVER_EVENT_NONE;
}

ObjObserverEventType objPollObservers(pXArray obss, int blocking, pObjObserver* updated, char** specificPath){
    return OBJ_OBSERVER_EVENT_NONE;
}
