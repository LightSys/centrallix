#ifndef _MTASK_H
#define _MTASK_H

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
/* Module:	MTASK Multithreading Module (mtask.c, mtask.h)          */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	May 1998                                                */
/*									*/
/* Description:	MTASK is a non-preemptive utility library that allows	*/
/*		for the use of a threaded programming model to make	*/
/*		life generally easier than writing a totally non-	*/
/*		threaded application.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: mtask.h,v 1.8 2002/11/12 00:26:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/mtask.h,v $

    $Log: mtask.h,v $
    Revision 1.8  2002/11/12 00:26:49  gbeeley
    Updated MTASK approach to user/group security when using system auth.
    The module now handles group ID's as well.  Changes should have no
    effect when running as non-root with altpasswd auth.

    Revision 1.7  2002/08/16 20:01:20  gbeeley
    Various autoconf fixes.  I hope I didn't break anything with this, being
    an autoconf rookie ;)

        * allowed config.h to really be included by adding @DEFS@ to the
          cflags in makefile.in.
        * moved config.h to include/cxlibconfig.h to prevent confusion
          between the various packages' config.h files.  Also, makedepend
          picks it up now because it is in the include directory.
        * fixed make depend to re-run when any source files have changed,
          just to be sure (the includes in the source files may have been
          modified if the timestamp has changed).

    Revision 1.6  2002/08/13 02:30:59  gbeeley
    Made several of the changes recommended by dman.  Most places where
    const char* would be appropriate have been updated to reflect that,
    plus a handful of other minor changes.  gzwrite() somehow is not
    happy with a const ____* buffer, so that is typecast to just void*.

    Revision 1.5  2002/07/31 18:36:05  mattphillips
    Let's make use of the HAVE_LIBZ defined by ./configure...  We asked autoconf
    to test for libz, but we didn't do anything with the results of its test.
    This wraps all the gzip stuff in #ifdef's so we will not use it if the system
    we built on doesn't have it.

    Revision 1.4  2002/07/23 02:30:54  jorupp
     (commiting for ctaylor)
     * removed unnecessary field from pFile and associated enum values
     * added a dup on the fd which eliminated a lot of checking
       -- we can close the fd without worry about the fd used by MTASK

    Revision 1.3  2002/07/21 04:52:51  jorupp
     * support for gzip encoding added by ctaylor
     * updated autoconf files to account for the new library (I think..)

    Revision 1.2  2002/05/03 03:46:29  gbeeley
    Modifications to xhandle to support clearing the handle list.  Added
    a param to xhClear to provide support for xhnClearHandles.  Added a
    function in mtask.c to allow the retrieval of ticks-since-boot without
    making a syscall.  Fixed an MTASK bug in the scheduler relating to
    waiting on timers and some modulus arithmetic.

    Revision 1.1.1.1  2001/08/13 18:04:19  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:00  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/

#include <setjmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef HAVE_CONFIG_H
#include "cxlibconfig.h"
#else
#define HAVE_LIBZ 1
#endif

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#ifdef MTASK_USEPTHREADS
#include <pthread.h>
#endif

/** General **/
#define FAIL 		<0
#define STR_LEN		32

#define FAILED		-1
#define OK		0

#define MT_F_NOYIELD	1		/* MTASK module shouldn't always yld */
#define MT_F_ONEPROC	2		/* Single-process optimization */
#define MT_F_LOCKED	4		/* Sched locked */
#define	MT_F_AUTOFLUSH	8		/* auto-flush buffers */

#define MT_FD_CACHE_SIZE	4096	/* 8kb */
#define MT_FD_FLUSH_INTERVAL	5000	/* 5 seconds */


/** EventCk Functions **/
int evThread(int event, void* object);
int evFile(int event, void* object);
int evRWLock(int event, void* object);
int evSem(int event, void* object);
int evMulti(int event, void* object);


/** Thread Extension Data **/
typedef struct _TE
    {
    char	Name[16];
    void*	Param;
    }
    ThrExt, *pThrExt;


/** THREAD data structure **/
typedef struct _THR
    {
    int		(*EventCkFn)(int,void*);	/* To check status of thread */
    char	Name[STR_LEN];			/* Name of thread */
    int		CurPrio;			/* Working priority */
    int		BasePrio;			/* Base priority */
    int		CntDown;			/* Usage calculation */
    int		Status;				/* THR_S_xxx */
    int		Flags;				/* one or more of THR_F_xxx */
    int		UserID;				/* ID of user of this thread */
    int		GroupID;			/* primary group of user */
#if 0
    pThrExt	ThrParam[16];			/* Structure extension */
#else
    void*	ThrParam;
#endif
    void	(*StartFn)();			/* start function */
    void*	StartParam;			/* Param to pass to start fn */
    int		BlkReturnCode;			/* Return code for longjmp */
    jmp_buf	SavedEnv;			/* for context switches */
    }
    Thread, *pThread;

#define THR_S_EXECUTING	1
#define THR_S_RUNNABLE	2
#define THR_S_BLOCKED	3

#define THR_F_STARTING	1
#define THR_F_IGNPIPE	2			/* ignore SIGPIPE */


/** File Descriptor data structure **/
typedef struct _FD
    {
    int		(*EventCkFn)(int,void*);	/* To check status of fd */
    int		FD;				/* OS fd or socket */
    int		Status;				/* FD_S_xxx */
    int		Flags;				/* FD_F_xxx */
    int		ErrCode;			/* From last op. */
    char	UnReadBuf[2048];
    int		UnReadLen;
    char*	UnReadPtr;
    struct sockaddr_in LocalAddr;
    struct sockaddr_in RemoteAddr;
    char*	WrCacheBuf;
    int		WrCacheLen;
    char*	WrCachePtr;
    char*	RdCacheBuf;
    int		RdCacheLen;
    char*	RdCachePtr;
#ifdef HAVE_LIBZ
    gzFile	GzFile;
#endif
    }
    File, *pFile;

#define FD_F_RDBLK	1		/* FD is blocked for reading */
#define FD_F_WRBLK	2		/* blocked for writing */
#define FD_F_RDERR	4		/* FD cannot be read -- err/eof */
#define FD_F_WRERR	8		/* FD cannot be written -- err/eof */
#define FD_F_RD		16		/* FD open for reading */
#define FD_F_WR		32		/* FD open for writing */
#define FD_UF_WRCACHE	64		/* Enable write-cache buffering */
#define FD_UF_RDCACHE	128		/* Enable read-cache buffering */
#define FD_UF_GZIP		256		/* Enable Gzip compression */


#define FD_S_OPENING	0		/* FD is opening or connecting */
#define FD_S_OPEN	1		/* FD is open normally */
#define	FD_S_ERROR	2		/* FD has errored unrecoverably */
#define FD_S_CLOSING	3		/* FD is closing */

/** user options **/
#define FD_U_NOBLOCK	1		/* User: call can't block. */
#define FD_U_SEEK	2		/* User: perform seek. */
#define FD_U_IMMEDIATE	4		/* for fdClose, error all pending */
#define FD_XU_NODST	8		/* fdClose, INTERNAL USE ONLY! */
#define FD_U_PACKET	16		/* r/w: do _only_whole_packet_ */

#define NET_U_NOBLOCK	1		/* User: net call can't block. */
#define NET_U_KEEPALIVE	32		/* User: keep net connection alive */

#define SEM_U_HARDCLOSE	1		/* User: semdestroy kills waiting events */
#define SEM_U_NOBLOCK	2		/* User: dont block on empty sem */

/** Event Request structure **/
typedef struct _EV
    {
    pThread	Thr;				/* Thread placing request */
    void*	Object;				/* Object for request */
    int		ObjType;			/* Type of object */
    int		EventType;			/* Type of event */
    int		Status;				/* EV_S_xxx */
    char*	ReqBuffer;			/* Buffer for request */
    int		ReqLen;				/* Requested length/count */
    int		ReqSeek;			/* Requested seek offset */
    int		ReqFlags;			/* Flags for request */
    unsigned long TargetTickCnt;		/* Sleep exit point */
    struct _EV*	NextPeer;			/* Next related event */
    int		TableIdx;			/* Index in event-wait-tbl */
    }
    EventReq, *pEventReq;

#define EV_S_INPROC	0		/* in process */
#define EV_S_COMPLETE	1		/* Complete.  For error, use - nums */
#define EV_S_ERROR	(-1)		/* Error/exception occurred */

#define OBJ_T_THREAD	0		/* Thread object, wait on start, etc */
#define OBJ_T_FD	1		/* File/socket descriptor */
#define OBJ_T_SEM	2		/* Semaphore */
#define OBJ_T_RWLOCK	3		/* Read/Write lock */
#define OBJ_T_MULTIOP	4		/* Multi-operation */
#define OBJ_T_MTASK	5		/* Internal object, for EV_T_MT_TIMER */

#define EV_T_THR_START	0		/* Wait on thread start */
#define EV_T_THR_EXIT	1		/* Wait on thread exit */
#define EV_T_THR_TIME	2		/* Wait 'til thread gets timeslice */
#define EV_T_FD_READ	3		/* Wait until fd is readable */
#define EV_T_FD_WRITE	4		/* Wait until fd is writable */
#define EV_T_FD_OPEN	5		/* Wait until fd is open/connected */
#define EV_T_SEM_GET	6		/* Wait until semaphore acquisition */
#define EV_T_RW_RDLOCK	7		/* Wait until readlock on a rw/lock */
#define EV_T_RW_WRLOCK	8		/* Wait until writelock on rw/lock */
#define EV_T_MULTI_AND	9		/* Wait until all of ops can be done */
#define EV_T_MT_TIMER	10		/* Wait until the timer runs out */

#define EV_T_FINISH	100		/* After others - used when closing */


/** Multi-op data structure **/
typedef struct _ML
    {
    int		(*EventCkFn)(int,void*);	/* Check if multiop complete */
    int		NumOps;				/* Number of operations */
    int		Flags;				/* Flags for this multiop */
    pEventReq	Ops;				/* The sub-operations */
    }
    MultiOp, *pMultiOp;

#define ML_F_AND	1		/* AND mode */


/** Semaphore data structure **/
typedef struct _SEM
    {
    int		(*EventCkFn)(int,void*);	/* Check if sem available */
    int		Count;				/* Semaphore count */
    int		Flags;				/* SEM_F_xxx */
    }
    Semaphore, *pSemaphore;

#define SEM_F_CLOSING	1		/* Semaphore is closing */


/** RWLock data structure **/
typedef struct _RW
    {
    int		(*EventCkFn)(int,void*);	/* Check if rwlock available */
    int		Status;				/* RW_S_xxx */
    int		Flags;				/* RW_F_xxx */
    int		ReaderCnt;			/* Number of reading threads */
    pSemaphore	ReaderSem;			/* Reader-wait semaphore */
    pSemaphore	WriterSem;			/* Writer-wait semaphore */
    }
    RWLock, *pRWLock;

#define RW_S_UNLOCKED	0		/* unlocked */
#define RW_S_RDLOCK	1		/* locked for reading */
#define RW_S_WRLOCK	2		/* locked for writing */

#define RW_F_CLOSING	1		/* rwlock is closing */
#define RW_F_FAST	2		/* rwlock closing FAST */


/** Generic object wrapper **/
typedef struct _OBJ
    {
    int		(*EventCkFn)(int,void*);	/* Event check function */
    }
    MTObject, *pMTObject;

#define PMTOBJECT(x) ((pMTObject)(x))


/** MTASK General Functions. **/
pThread mtInitialize(int flags, void (*start_fn)());
unsigned long mtRealTicks();
unsigned long mtTicks();
unsigned long mtLastTick();


/** MTASK Threading functions. **/
pThread thCreate(void (*start_fn)(), int priority, void* start_param);
int thYield();
int thExit();
int thKill(pThread thr);
int thWait(pMTObject obj, int obj_type, int event_type, int arg_count);
int thMultiWait(int event_cnt, pEventReq event_req[]);
char* thGetName(pThread thr);
pThread thCurrent();
int thSetName(pThread thr, const char* new_name);
int thGetThreadList(int max_cnt, char* names[], int stati[], int flags[]);
pThread thGetByName(const char* name);
int thGetPrio(pThread thr);
int thSetPrio(pThread thr, int prio);
int thLock();
int thUnlock();
int thSleep(int msec);
int thSetUserID(pThread thr, int new_uid);
int thGetUserID(pThread thr);
int thSetGroupID(pThread thr, int new_gid);
int thGetGroupID(pThread thr);
int thSetParam(pThread thr, const char* name, void* param);
void* thGetParam(pThread thr, const char* name);
int thSetFlags(pThread thr, int flags);
int thClearFlags(pThread thr, int flags);


/** MTASK File i/o functions **/
pFile fdOpen(const char* filename, int mode, int create_mode);
pFile fdOpenFD(int fd, int mode);
int fdRead(pFile filedesc, char* buffer, int maxlen, int offset, int flags);
int fdUnRead(pFile filedesc, const char* buffer, int length, int offset, int flags);
int fdWrite(pFile filedesc, const char* buffer, int length, int offset, int flags);
int fdClose(pFile filedesc, int flags);
int fdFD(pFile filedesc);
int fdSetOptions(pFile filedesc, int options);
int fdUnSetOptions(pFile filedesc, int options);


/** MTASK Networking Functions **/
pFile netListenTCP(const char* service_name, int queue_length, int flags);
pFile netAcceptTCP(pFile net_filedesc, int flags);
pFile netConnectTCP(const char* host_name, const char* service_name, int flags);
int netCloseTCP(pFile net_filedesc, int linger_msec, int flags);
char* netGetRemoteIP(pFile net_filedesc, int flags);
unsigned short netGetRemotePort(pFile net_filedesc);


/** MTASK Synchronization Functions **/
pSemaphore syCreateSem(int init_count, int flags);
int syDestroySem(pSemaphore sem, int flags);
int syPostSem(pSemaphore sem, int cnt, int flags);
int syGetSem(pSemaphore sem, int cnt, int flags);
pRWLock syCreateRW(int init_state, int flags);
int syDestroyRW(pRWLock rw_lock, int flags);
int syReadLock(pRWLock rw_lock, int flags);
int syWriteLock(pRWLock rw_lock, int flags);
int syUnlock(pRWLock rw_lock, int flags);
int syMultiOp(pMultiOp multi_ops, int flags);


#endif /* _MTASK_H */
