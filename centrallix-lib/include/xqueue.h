#ifndef _XQUEUE_H
#define _XQUEUE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:	xqueue.h, xqueue.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	May 26th, 2000						*/
/* Description:	Provides a queue management system, basically a doubly	*/
/*		linked list data structure.  Very simple, and macro-	*/
/*		based.							*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: xqueue.h,v 1.1 2001/08/13 18:04:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/xqueue.h,v $

    $Log: xqueue.h,v $
    Revision 1.1  2001/08/13 18:04:20  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/07/03 01:03:03  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


/** Queue Structure **/
typedef struct _XQ
    {
    struct _XQ*	Next;
    struct _XQ*	Prev;
    struct _XQ* Head;
    }
    XQueue, pXQueue;


/** Macros for enqueue/dequeue **/
#define xqInit(q) (((q)->Next=(q)),((q)->Prev=(q)),((q)->Head=(q)))
#define xqAddHead(q,e) (((e)->Next=(q)->Next),((e)->Prev=(q)),((e)->Prev->Next=(e)),((e)->Next->Prev=(e)),((e)->Head=(q)))
#define xqAddAfter(q,e) (((e)->Next=(q)->Next),((e)->Prev=(q)),((e)->Prev->Next=(e)),((e)->Next->Prev=(e)),((e)->Head=(q)))
#define xqAddTail(q,e) (((e)->Next=(q)),((e)->Prev=(q)->Prev),((e)->Prev->Next=(e)),((e)->Next->Prev=(e)),((e)->Head=(q)))
#define xqAddBefore(q,e) (((e)->Next=(q)),((e)->Prev=(q)->Prev),((e)->Prev->Next=(e)),((e)->Next->Prev=(e)),((e)->Head=(q)))
#define xqRemove(e) (((e)->Prev->Next=(e)->Next),((e)->Next->Prev=(e)->Prev),((e)->Next=NULL),((e)->Prev=NULL),((e)->Head=NULL))
#define xqDeInit(q) (((q)->Next=NULL),((q)->Prev=NULL),((q)->Head=NULL))
/* #define xqGetStruct(q,n,t) ((t)(((char*)q)-(((char*)(&(((t)q)->n)))-((char*)(q))))) */
#define xqGetStruct(q,n,t) ((t)(((char*)q)-((unsigned int)(&(((t)0)->n)))))

#endif /* _XQUEUE_H */
