#ifndef MTNOTIFY_H
#define	MTNOTIFY_H
/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	mtnotify.h      					*/
/* Author:	Micah Shennum and Daniel Rothfus			*/
/* Creation:	July 21, 2011   					*/
/* Description:	A thread notification system that simplifies the        */
/*              delivery of events to threads that are waiting on them.	*/
/************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif
#ifdef SHOULDNOTBEDEFINED
}
#endif

#ifdef CXLIB_INTERNAL
#include "xarray.h"
#include "xhandle.h"
#else
#include "cxlib/xarray.h"
#include "cxlib/xhandle.h"
#endif

/** \addgroup mtn Notifcation System
 * \brief This system handles blocking notifications to threads.
 * \{
 */

/** \brief The type that is used for referencing the different kinds of events
 * that are passed by the notification system.
 */
typedef handle_t registered_event;

#define ET_PHONESHOME    0
#define ET_HEALS         0
#define ET_BLOCKS        0
#define ET_NOTBLOCKS     1
#define ET_POLL         (1<<1)

typedef struct _EV{
    int flags;
    /**@brief priority **/
    int priority;
    pXArray paramArray;
    char *type;
} Event, *pEvent;

///@brief Initializes the mtNotify system
int mtnInitialize(int flags);

///@brief Cleans up after the mtNotify system and sends necessary events to quit
/// threads that are waitng on events.
int mtnDeinitialize();

/** \brief Create a new event of a given type.
 * \param typeOfEvent The type of event to create a new event of.
 * \return This returns the new event that was created.
 */
pEvent mtnCreateEvent(char *typeOfEvent, int priority);

/** \brief Delete an event structure - NOTE THAT THIS SHOULD ONLY BE CALLED BY
 THE RECIEVER OF THE EVENT, NOT THE SENDER!
 \param event The event to delete.
 */
void mtnDeleteEvent(pEvent event);

/** \brief Send an event out to the necessary handler.
 * \param event The event to send out to some receiver.
 */
int mtnSendEvent(pEvent event);

/** \brief This is called by the thread that wants to receive the event when it
 happens.
 \param events The array of events to wait for.
 \return This returns the event that finally occurred.
 */
pEvent mtnWaitForEvent(pXArray events);

///\}

#ifdef	__cplusplus
}
#endif

#endif	/* MTNOTIFY_H */

