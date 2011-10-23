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



/** Queue Structure **/
typedef struct _XQ
    {
    struct _XQ*	Next;
    struct _XQ*	Prev;
    struct _XQ* Head;
    }
    XQueue, pXQueue;

/** Macros for enqueue/dequeue **/
#define xqInit(q) (((q).Next=&(q)),((q).Prev=&(q)),((q).Head=&(q)))
#define xqAddHead(q,e) (((e).Next=(q).Next),((e).Prev=&(q)),((e).Prev->Next=&(e)),((e).Next->Prev=&(e)),((e).Head=&(q)))
#define xqAddAfter(q,e) (((e).Next=(q).Next),((e).Prev=&(q)),((e).Prev->Next=&(e)),((e).Next->Prev=&(e)),((e).Head=&(q)))
#define xqAddTail(q,e) (((e).Next=&(q)),((e).Prev=(q).Prev),((e).Prev->Next=&(e)),((e).Next->Prev=&(e)),((e).Head=&(q)))
#define xqAddBefore(q,e) (((e).Next=&(q)),((e).Prev=(q).Prev),((e).Prev->Next=&(e)),((e).Next->Prev=&(e)),((e).Head=&(q)))
#define xqRemove(e) (((e).Prev->Next=(e).Next),((e).Next->Prev=(e).Prev),((e).Next=NULL),((e).Prev=NULL),((e).Head=NULL))
#define xqDeInit(q) (((q).Next=NULL),((q).Prev=NULL),((q).Head=NULL))
/* #define xqGetStruct(q,n,t) ((t)(((char*)q)-(((char*)(&(((t)q)->n)))-((char*)(q))))) */
#define xqGetStruct(q,n,t) ((t)(((char*)&(q))-((unsigned int)(&(((t)0)->n)))))

#define xqHead(q)	(((q).Next==&(q))?NULL:((q).Next))
#define xqTail(q)	(((q).Prev==&(q))?NULL:((q).Prev))

#endif /* _XQUEUE_H */
