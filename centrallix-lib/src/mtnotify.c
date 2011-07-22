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

int mtnInitialize(){
}

int mtnDeinitialize(){
}

pEvent mtnCreateEvent(pXString typeOfEvent, void * data, int priority, int flags){
}

void mtnDeleteEvent(pEvent event){
}

int mtnSendEvent(pEvent event){
}

pEvent mtnWaitForEvents(pXArray eventStrings, int blocking, int prioity ){
}
