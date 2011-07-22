#ifndef MTNOTIFY_H
#define MTNOTIFY_H

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      mtnotify.h                                              */
/* Author:      Micah Shennum and Daniel Rothfus                        */
/* Creation:    July 21, 2011                                           */
/* Description: A thread notification system that simplifies the        */
/*              delivery of events to threads that are waiting on them. */
/************************************************************************/

#ifdef    __cplusplus
extern "C" {
#endif

#ifdef CXLIB_INTERNAL
#include "xarray.h"
#include "xhandle.h"
#else
#include "cxlib/xarray.h"
#include "cxlib/xhandle.h"
#endif

/** @addgroup mtn Notifcation System
 * @brief This system handles blocking notifications to threads.
 * @{
 */

/** @addgroup mtn_flags Notification Event Flags
 * @brief All the valid flags that can be put into an Event structure.
 * @{
 */
#define ET_PHONESHOME    0
#define ET_HEALS         1
/// @}

/** @brief This is the structure that is passed all over when notfiying of events.
 *
 * These should always be created with mtnCreateEvent and deleted with mtnDeleteEvent
 * whenever they are recieved from mtnWaitForEvents.
 **/
typedef struct _EV{
    /** @brief Do not modify - reference counting data for an event. */
    unsigned short refcount;
    /** @brief These are some of the ET_* options. */
    int             flags;
    /**@brief The priority of the event.
     * The lower priority will trump those of higher values. 
     **/
    unsigned short   priority;
    /** @brief The data you want to pass with the event.
     *
     * mtNotify makes no assumtions on the types of this data, the pointer will
     * be completely unmodified, and will not be freed during mtnDeleteEvent.
     **/
    void            *param;
    /** @brief Events are matched up from their sender to their receiver with this.
     *
     * There are a few disallowed names, specifically "quit" and "none".
     **/
    pXString        type;
} Event, *pEvent;

///@brief Initializes the mtNotify system
int mtnInitialize();

/** @brief Cleans up after the mtNotify system and sends necessary events to quit
 * threads that are waitng on events.
 **/
int mtnDeinitialize();

/** @brief Create a new event of a given type.
 * @param typeOfEvent A string designating the type of event.
 * @param data The data to pass with the event.  You are responsible for taking 
 * care of the memory management of this data and knowing what type it is of.
 * @param priority The lower this is, the more clout the event has.  0 is the 
 * best that it can get.
 * @param flags The flags for the event to affect delivery and such.
 * @return This returns the new event that was created or NULL if the event could
 * not be created.
 */
pEvent mtnCreateEvent(pXString typeOfEvent, void * data, int priority, int flags);

/** @brief Delete an event structure 
 * @note NOTE THAT THIS SHOULD ONLY BE CALLED BY THE RECIEVER OF THE EVENT, NOT 
 * THE SENDER! 
 * @note ALSO, YOU MUST CALL THIS ON EVERY EVENT YOU RECIEVE FROM mtnWaitForEvent!
 * @param event The event to delete.
 */
void mtnDeleteEvent(pEvent event);

/** @brief Send an event out to the necessary handler.
 * @param event The event to send out to some receiver.
 */
int mtnSendEvent(pEvent event);

/** @brief This is called by the thread that wants to receive the event when it
 * happens.
 * @param eventStrings An array of strings of events to wait for.
 * @param blocking If this call should block until there are meaningful return values.
 * @return This returns the event that finally occurred.
 * @TODO allow user to enter time to give up after
 **/
pEvent mtnWaitForEvents(pXArray eventStrings, int blocking, int prioity );

///\}

#ifdef    __cplusplus
}
#endif

#endif    /* MTNOTIFY_H */