#ifndef _XRINGQUEUE_H
#define _XRINGQUEUE_H

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
/* Module: 	xringqueue.c, xringqueue.h				*/
/* Author:	Jonathan Rupp (JDR)					*/
/* Creation:	Match 13, 2003	    					*/
/* Description:	Extensible ring queue system for Centrallix.  Provides	*/
/*		for variable-length 32-bit value ring queues.  On Alpha */
/*		systems, values are 64-bit, but can still store 32-bit	*/
/*		values.	(I think)					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xringqueue.h,v 1.1 2003/03/15 06:50:47 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xringqueue.h,v $

    $Log: xringqueue.h,v $
    Revision 1.1  2003/03/15 06:50:47  jorupp
     * added XRingQueue -- a queue based on an auto-expanding ring buffer


 **END-CVSDATA***********************************************************/

/** Structure **/
typedef struct
    {
    int		nextIn;
    int		nextOut;
    int		nAlloc;
    void**	Items;
    }
    XRingQueue, *pXRingQueue;

/** Functions **/
int xrqInit(pXRingQueue this, int initSize);
int xrqDeInit(pXRingQueue this);
int xrqEnqueue(pXRingQueue this, void* item);
void* xrqDequeue(pXRingQueue this);
int xrqClear(pXRingQueue this);
int xrqCount(pXRingQueue this);

#endif /* _XRINGQUEUE_H */
