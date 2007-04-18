#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/times.h>
#include <sys/mman.h>
#include <time.h>
#include <termios.h>
#include <netdb.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef LINUX22
#include <ioctls.h>
#endif
#include <grp.h>
#include <stdarg.h>
#include "qprintf.h"

#ifdef USING_VALGRIND
#include "valgrind/valgrind.h"
#endif

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
/* Module:	MTASK Multithreading Tasking Module (mtask.c,mtask.h)   */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	May 1998                                                */
/*									*/
/* Description:								*/
/*									*/
/* The MTASK Multithreading Tasking Module provides non-preemptive	*/
/* threading services for Centrallix.  It has been shown to be useable	*/
/* on a variety of platforms, although the values for MT_TASKSEP may	*/
/* sometimes need to be adjusted.  This module does NOT provide for	*/
/* kernel threads or for preemptive threading.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: mtask.c,v 1.38 2007/04/18 18:42:07 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/mtask.c,v $

    $Log: mtask.c,v $
    Revision 1.38  2007/04/18 18:42:07  gbeeley
    - (feature) hex encoding in qprintf (&HEX filter).
    - (feature) auto addition of quotes (&QUOT and &DQUOT filters).
    - (bugfix) %[ %] conditional formatting didn't exclude everything.
    - (bugfix) need to ignore, rather than error, on &nbsp; following filters.
    - (performance) significant performance improvements in HEX, ESCQ, HTE.
    - (change) qprintf API change - optional session, cumulative errors/flags
    - (testsuite) lots of added testsuite entries.

    Revision 1.37  2007/04/08 03:43:06  gbeeley
    - (bugfix) some code quality fixes
    - (feature) MTASK integration with the Valgrind debugger.  Still some
      problems to be sorted out, but this does help.  Left to themselves,
      MTASK and Valgrind do not get along, due to the approach to threading.

    Revision 1.36  2007/03/06 03:51:57  gbeeley
    - (security) Adding a function to aid in recursion depth limiting, since
      stack space is at a premium when using MTask.

    Revision 1.35  2007/02/20 19:32:16  gbeeley
    - (bugfix #757726) Compile properly without zlib support.  This is a
      really ancient build bug that we should have fixed eons ago...

    Revision 1.34  2006/06/21 21:25:10  gbeeley
    - Adding fdQPrintf() routines to utilize the qpfPrintf routines.

    Revision 1.33  2006/04/07 06:14:21  gbeeley
    - (bugfix) memory_leaks -= 2;

    Revision 1.32  2005/10/12 03:40:16  gbeeley
    - (bugfix) don't linger for 100 sec when asked to do so for 1 sec!
    - (bugfix) if select()ing with no timeout, make sure it is really
      0 seconds / 0 usec - code was wasting a tick here and there.

    Revision 1.31  2005/10/09 00:01:05  gbeeley
    - (bugfix) better protection against "deadlocks" caused by clock skew.

    Revision 1.30  2005/03/14 20:33:35  gbeeley
    - changing the interface to the get thread list function so that we can
      better identify a specific thread from one call to another.

    Revision 1.29  2005/02/06 02:35:41  gbeeley
    - Adding 'mkrpm' script for automating the RPM build process for this
      package (script is portable to other packages).
    - stubbed out pipe functionality in mtask (non-OS pipes; to be used
      between mtask threads)
    - added xsString(xstr) for getting the string instead of xstr->String.

    Revision 1.28  2004/08/30 02:29:50  gbeeley
    - SnNode magic number
    - fixed that constant with an, ummm, 'ambiguous' value

    Revision 1.27  2004/08/30 01:36:40  jorupp
     * add a bit more debugging information to non-IO selects
         this should help diagnose the current issue with cygwin

    Revision 1.26  2004/08/29 05:35:21  jorupp
     * fixed an overflow error under cygwin, where MTASK.TicksPerSec is 1000

    Revision 1.25  2004/06/12 04:09:37  gbeeley
    - supporting logic to allow saving of an MTask security context for later
      use in a new thread.  This is needed for the asynchronous event delivery
      mechanism for object-updates being sent to the client.

    Revision 1.24  2004/05/04 18:18:59  gbeeley
    - Adding fdAccess() wrapper for access(2).
    - Moving PTOD definition to module in centrallix core.

    Revision 1.23  2004/02/24 05:09:10  gbeeley
    - fixed error in mtask's handling of group id.
    - renamed WRCACHE/RDCACHE to WRBUF/RDBUF.

    Revision 1.22  2003/06/05 04:22:54  jorupp
     * added support for mTask-based signal handlers
       - go ahead greg -- pick apart my implimentation.....

    Revision 1.21  2003/04/03 04:32:39  gbeeley
    Added new cxsec module which implements some optional-use security
    hardening measures designed to protect data structures and stack
    return addresses.  Updated build process to have hardening and
    optimization options.  Fixed some build-related dependency checking
    problems.  Updated mtask to put some variables in registers even
    when not optimizing with -O.  Added some security hardening features
    to xstring as an example.

    Revision 1.20  2003/03/29 08:32:47  jorupp
     * fixed a bug regarding possibly not delaying long enough in a select() call
     * redid some of the actual values for debugging -- the bit groups make a bit more sense now
     * added debugging code for the checking of timers

    Revision 1.19  2003/03/09 18:58:45  jorupp
     * change to allow the scheduler (or the interrupt handler interupting the
    	 scheduler) to spawn new threads

    Revision 1.18  2003/03/07 19:30:57  gbeeley
    Fixed some retry problems with read/write/recvfrom/sendto dealing
    with the EINTR error condition which should be considered a retryable
    error in most cases, so we do a retry.  Was having trouble with
    MTask writes to parallel port causing EINTR under nonblocking I/O,
    corrupting the data sent to the printer.

    Revision 1.17  2003/03/03 02:12:03  jorupp
     * it always helps when you put the brace on the correct side.... :)

    Revision 1.16  2003/02/20 23:16:44  jorupp
     * mtask debugging is not enabled by default
     * added configure option to turn it on

    Revision 1.15  2003/02/20 22:57:42  jorupp
     * added quite a bit of debugging to mTask
     	* call mtSetDebug() to set debugging level
     	* undefine MTASK_DEBUG to disable
     * added UDP support (a basic echo client/server is working with it)
     * changed pFile returned by netConnectTCP to include IP and port of remote computer

    Revision 1.14  2002/11/22 20:56:58  gbeeley
    Added xsGenPrintf(), fdPrintf(), and supporting logic.  These routines
    basically allow printf() style functionality on top of any xxxWrite()
    type of routine (such as fdWrite, objWrite, etc).

    Revision 1.13  2002/11/12 00:26:49  gbeeley
    Updated MTASK approach to user/group security when using system auth.
    The module now handles group ID's as well.  Changes should have no
    effect when running as non-root with altpasswd auth.

    Revision 1.12  2002/09/24 18:59:17  gbeeley
    Fixed some problems relating to non-blocking netConnectTCP() operations
    (where FD_U_NOBLOCK was specified).  MTask was not correctly retrieving
    the SO_ERROR value once writability was confirmed on the socket
    (writability is the indication of completion, whether successful or
    not).  Note: the connect is always internally nonblocking, but if the
    FD_U_NOBLOCK flag is given, then the call returns to the user before
    the connect has completed.  Do a Wait or MultiWait for writability to
    determine whether it has completed, and check the event status for
    successful completion.

    Revision 1.11  2002/09/20 20:38:08  jorupp
     * netAcceptTCP now sets the returned pFile to non-blocking IO

    Revision 1.10  2002/08/16 03:05:38  jorupp
     * added checks for shadow passwords and endservent() to allow build under cygwin
       -- this has been tested under cygwin, and it works pretty well

    Revision 1.9  2002/08/13 02:38:24  gbeeley
    Added "plan B" in case CLK_TCK ever becomes obsolete.  Linux man page
    says CLK_TCK is a macro for the sysconf(_SC_CLK_TCK) call.  The sysconf
    call will be used if CLK_TCK ever disappears.

    Revision 1.8  2002/08/13 02:30:59  gbeeley
    Made several of the changes recommended by dman.  Most places where
    const char* would be appropriate have been updated to reflect that,
    plus a handful of other minor changes.  gzwrite() somehow is not
    happy with a const ____* buffer, so that is typecast to just void*.

    Revision 1.7  2002/07/31 18:36:05  mattphillips
    Let's make use of the HAVE_LIBZ defined by ./configure...  We asked autoconf
    to test for libz, but we didn't do anything with the results of its test.
    This wraps all the gzip stuff in #ifdef's so we will not use it if the system
    we built on doesn't have it.

    Revision 1.6  2002/07/23 02:30:55  jorupp
     (commiting for ctaylor)
     * removed unnecessary field from pFile and associated enum values
     * added a dup on the fd which eliminated a lot of checking
       -- we can close the fd without worry about the fd used by MTASK

    Revision 1.5  2002/07/21 04:52:51  jorupp
     * support for gzip encoding added by ctaylor
     * updated autoconf files to account for the new library (I think..)

    Revision 1.4  2002/06/18 16:25:49  gbeeley
    Fixed a couple of warnings that showed up with -O2.  Fixed an MTASK bug
    related to thMultiWait().

    Revision 1.3  2002/05/03 03:46:29  gbeeley
    Modifications to xhandle to support clearing the handle list.  Added
    a param to xhClear to provide support for xhnClearHandles.  Added a
    function in mtask.c to allow the retrieval of ticks-since-boot without
    making a syscall.  Fixed an MTASK bug in the scheduler relating to
    waiting on timers and some modulus arithmetic.

    Revision 1.2  2001/12/28 21:52:56  gbeeley
    netCloseTCP(fd,0,0), which causes a hard close, was leaving the socket
    fd still open after deallocating the structure.  Fixed.

    Revision 1.1.1.1  2001/08/13 18:04:21  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:02:50  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/

#include "newmalloc.h"
#include "mtask.h"
#include "xstring.h"
#include "xhash.h"
#include "xringqueue.h"

/** Internal stuff **/
int mtRunStartFn(pThread new_thr, int idx);
int r_mtRunStartFn();
int mtSched();

#define MAX_EVENTS		256
#define MAX_THREADS		256
#define MAX_STACK		(31 * 1024)
#define MT_STACK_HIGHWATER	(24 * 1024)
#define MT_TASKSEP		256
#define MT_TICK_MAX		1

//#define dbg_write(x,y,z) write(x,y,z)
#define dbg_write(x,y,z)

typedef struct _MTS
    {
    pEventReq	EventWaitTable[MAX_EVENTS];
    pThread	ThreadTable[MAX_THREADS];
    int 	nThreads;
    int		nEvents;
    int		MTFlags;
    pThread	CurrentThread;
    int		TickCnt;
    int		FirstTick;
    int		TicksPerSec;
    int		StartUserID;
    int		CurUserID;
    int		CurGroupID;
    unsigned long LastTick;
    unsigned int DebugLevel;
    XRingQueue	PendingSignals;
    XHashTable	SignalHandlers;
    }
    MTSystem, *pMTSystem;

static MTSystem MTASK;

#ifdef MTASK_DEBUG
#define MTASK_DEBUG_SHOW_READ_SELECTABLE 0x01
#define MTASK_DEBUG_SHOW_WRITE_SELECTABLE 0x02
#define MTASK_DEBUG_SHOW_ERROR_SELECTABLE 0x04
#define MTASK_DEBUG_SHOW_TIMER_SELECTABLE 0x08
#define MTASK_DEBUG_SHOW_READ_SELECTED 0x10
#define MTASK_DEBUG_SHOW_WRITE_SELECTED 0x20
#define MTASK_DEBUG_SHOW_ERROR_SELECTED 0x40
#define MTASK_DEBUG_SHOW_TIMER_SELECTED 0x80
#define MTASK_DEBUG_SHOW_CONNECTION_OPEN 0x100
#define MTASK_DEBUG_SHOW_CONNECTION_CLOSE 0x200
#define MTASK_DEBUG_FDOPEN 0x400
#define MTASK_DEBUG_FDCLOSE 0x800
#define MTASK_DEBUG_SHOW_IO_SELECT 0x1000
#define MTASK_DEBUG_SHOW_NON_IO_SELECT 0x2000
#define MTASK_DEBUG_SHOW_SELECT_TICKS_USED 0x4000
#else
#define MTASK_DEBUG_SHOW_READ_SELECTABLE 0
#define MTASK_DEBUG_SHOW_WRITE_SELECTABLE 0
#define MTASK_DEBUG_SHOW_ERROR_SELECTABLE 0
#define MTASK_DEBUG_SHOW_TIMER_SELECTABLE 0
#define MTASK_DEBUG_SHOW_READ_SELECTED 0
#define MTASK_DEBUG_SHOW_WRITE_SELECTED 0
#define MTASK_DEBUG_SHOW_ERROR_SELECTED 0
#define MTASK_DEBUG_SHOW_TIMER_SELECTED 0
#define MTASK_DEBUG_SHOW_CONNECTION_OPEN 0
#define MTASK_DEBUG_SHOW_CONNECTION_CLOSE 0
#define MTASK_DEBUG_FDOPEN 0
#define MTASK_DEBUG_FDCLOSE 0
#define MTASK_DEBUG_SHOW_IO_SELECT 0
#define MTASK_DEBUG_SHOW_NON_IO_SELECT 0
#define MTASK_DEBUG_SHOW_SELECT_TICKS_USED 0
#endif

/*** mtSetDebug - sets the debugging level ***/
void
mtSetDebug(int debuglevel)
    {
#ifdef MTASK_DEBUG
    MTASK.DebugLevel=debuglevel;
    printf("DEBUG: %i\n",MTASK.DebugLevel);
#endif
    }

/*** EVSEMAPHORE is an event processor for semaphores ***/
int
evSemaphore(int ev_type, void* obj)
    {
    pSemaphore sem = (pSemaphore)obj;
    int code = -1;

    	switch(ev_type)
	    {
	    case EV_T_SEM_GET:
	        if (sem->Count > 0) 
		    {
		    code = 1;
		    sem->Count--;
		    }
		else 
		    {
		    code = 0;
		    }
		break;
	    }

    return code;
    }


/*** EVTHREAD is an event processor for threads. ***/
int
evThread(int ev_type, void* obj)
    {
    pThread thr = (pThread)obj;
    int code = -1;

    	switch(ev_type)
	    {
	    case EV_T_THR_START:
	        code=(thr->Flags & THR_F_STARTING)?0:1;
		break;
		    
	    case EV_T_THR_TIME:
	        code=0;
		break;

	    case EV_T_THR_EXIT:
	        code=0;
		break;
	    }

    return code;
    }


/*** EVFILE is an event processor for files. ***/
int
evFile(int ev_type, void* obj)
    {
    pFile fd = (pFile)obj;
    int code = 0;
    fd_set readfds,writefds,exceptfds;
    struct timeval tmout;
    int arg,len;

    	if (fd->Status == FD_S_CLOSING) return -1;

	tmout.tv_sec = 0;
	tmout.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

        switch(ev_type)
            {
            case EV_T_FD_READ:
                if (!(fd->Flags & FD_F_RD)) break;
                FD_SET(fd->FD,&readfds);
                FD_SET(fd->FD,&exceptfds);
                select(fd->FD+1,&readfds,&writefds,&exceptfds,&tmout);
                if (FD_ISSET(fd->FD,&readfds)) code=1;
                if (FD_ISSET(fd->FD,&exceptfds)) code=-1;
                if (code == 0) fd->Flags |= FD_F_RDBLK;
                if (code == 1) fd->Flags &= ~(FD_F_RDBLK | FD_F_RDERR);
                if (code == -1) fd->Flags |= FD_F_RDERR;
                break;

            case EV_T_FD_OPEN:
                code=(fd->Status == FD_S_OPEN)?1:0;
                if (code==1) break;
                /** fallthrough and check for writability as check for open **/

            case EV_T_FD_WRITE:
                if (!(fd->Flags & FD_F_WR)) break;
                FD_SET(fd->FD,&writefds);
                FD_SET(fd->FD,&exceptfds);
                select(fd->FD+1,&readfds,&writefds,&exceptfds,&tmout);
                if (FD_ISSET(fd->FD,&writefds)) code=1;
		if (fd->Status == FD_S_OPENING && getsockopt(fd->FD, SOL_SOCKET, SO_ERROR, (void*)&arg, &len) >= 0) 
		    {
		    if (arg == EINPROGRESS)
			code = 0;
		    else if (arg == 0)
			code = 1;
		    else
			code = -1;
		    }
                if (FD_ISSET(fd->FD,&exceptfds)) code=-1;
                if (code == 0) fd->Flags |= FD_F_WRBLK;
                if (code == 1) fd->Flags &= ~(FD_F_WRBLK | FD_F_WRERR);
                if (code == -1) fd->Flags |= FD_F_WRERR;
                break;
            }

    return code;
    }


/*** EVPIPE is an event processor for pipes
 ***/
int
evPipe(int ev_type, void* obj)
    {
    }


/*** EVREMOVEIDX removes an event from the event wait table, keeping the oldest
 *** events entered at the front of the table.
 ***/
pEventReq
evRemoveIdx(int idx)
    {
    pEventReq event;
    register int i;

    	event = MTASK.EventWaitTable[idx];
	MTASK.nEvents--;
	for(i=idx;i<MTASK.nEvents;i++) 
	    {
	    MTASK.EventWaitTable[i] = MTASK.EventWaitTable[i+1];
	    MTASK.EventWaitTable[i]->TableIdx = i;
	    }
	MTASK.EventWaitTable[MTASK.nEvents+1] = NULL;

    return event;
    }


/*** EVREMOVE removes an event based on its pointer, not index.  Same as above
 *** otherwise.
 ***/
int
evRemove(pEventReq event)
    {
    register int i, j, k = -1;

    	for(i=0;i<MTASK.nEvents;i++) if (MTASK.EventWaitTable[i] == event)
	    {
	    MTASK.nEvents--;
	    for(j=i;j<MTASK.nEvents;j++) 
	        {
	        MTASK.EventWaitTable[j] = MTASK.EventWaitTable[j+1];
	        MTASK.EventWaitTable[j]->TableIdx = j;
		}
	    k = i;
	    break;
	    }
	MTASK.EventWaitTable[MTASK.nEvents+1] = NULL;

    return k;
    }


/*** EVADD adds an event to the event wait table, keeping the oldest events
 *** entered at the front of the table.
 ***/
int
evAdd(pEventReq event)
    {
    if (MTASK.nEvents >= MAX_EVENTS)
        {
	printf("mtask: Event table exhausted (%d)\n",MAX_EVENTS);
	return -1;
	}
    MTASK.EventWaitTable[MTASK.nEvents++] = event;
    return MTASK.nEvents-1;
    }


/*** MTSIGPIPE processes broken-pipe signals for threads.  If current thread
 *** is set, we can assume that the thread in question caused the sigpipe.
 *** If so, we kill that thread.  Otherwise, we ignore it.
 ***/
void
mtSigPipe()
    {
    signal(SIGPIPE,mtSigPipe);
    if (MTASK.CurrentThread && !(MTASK.CurrentThread->Flags & THR_F_IGNPIPE)) 
        {
	printf("Thread %s received SIGPIPE, exiting.\n",MTASK.CurrentThread->Name);
	thExit();
	}
    return;
    }


/*** MTSIGSEGV processes segmentation fault signals for threads.  IF current
 *** thread is set, it caused the SIGSEGV.  Otherwise, the SIGSEGV occurred during
 *** the mtSched operation, in which case the process must croak.  If not, simply
 *** terminate the erring thread.
 ***/
void
mtSigSegv()
    {
    if (MTASK.CurrentThread)
        {
	printf("Thread %s received SIGSEGV, terminating.\n", MTASK.CurrentThread->Name);
        signal(SIGSEGV,mtSigSegv);
	thExit();
	}
    else
        {
	printf("Scheduler received SIGSEGV, exiting process immediately!\n");
	_exit(1);
	}
    return;
    }

/*** mtTrapSignal is the signal handler for every signal that has an application-level
 ***   MTASK signal handler installed for it -- it simply queues the signals for processing
 ***/
void mtTrapSignal(int signum)
    {
    int* sig;
    sig = (int*)nmMalloc(sizeof(int));
    if(!sig)
	{
	printf("Could not allocate memory to handle signal: %i\n",signum);
	return;
	}
    *sig = signum;
    xrqEnqueue(&MTASK.PendingSignals,sig);
    }


/*** mtAddSignalHandler is called from user code to register a signal handler for the whole application
 ***   returns 0 on success and -1 on failure
 ***   signum is the signal to catch, and start_fn is the function to use to start the thread to handle it
 *** NOTE: signal handlers are _not_ protected from having more than one running at once -- you
 ***   must use semaphores in start_fn if you want to guarantee this
 ***/
int mtAddSignalHandler(int signum, void(*start_fn)())
    {
    pXArray list;

    /** can't trap SIGPIPE or SIGSEGV right now **/
    if(signum == SIGPIPE || signum == SIGSEGV)
	{
	return -1;
	}
    
    list = (pXArray)xhLookup(&MTASK.SignalHandlers,(char*)&signum);
    if(!list)
	{
	int* sig;
	sig = (int*)nmMalloc(sizeof(int));
	if(!sig)
	    {
	    return -1;
	    }
	*sig = signum;
	list = (pXArray)nmMalloc(sizeof(XArray));
	if(!list)
	    {
	    nmFree(sig,sizeof(int));
	    return -1;
	    }
	xaInit(list,4);
	xhAdd(&MTASK.SignalHandlers,(char*)sig, (char*)list);
	}
    if(xaAddItem(list,start_fn)<0)
	{
	return -1;
	}
    else
	{
	signal(signum,mtTrapSignal);
	return 0;
	}
    }

/*** mtRemoveSignalHandler is called from user code to remove a signal handler for the whole application
 ***   returns 0 on success, -1 if the signal is not registered, and -2 if unable to remove
 ***/
int mtRemoveSignalHandler(int signum, void(*start_fn)())
    {
    pXArray list;
    int i;
    list = (pXArray) xhLookup(&MTASK.SignalHandlers,(char*)&signum);
    if(!list)
	{
	return -1;
	}
    for(i=0;i<xaCount(list);i++)
	{
	if(xaGetItem(list,i)==start_fn)
	    {
	    if(xaRemoveItem(list,i)<0)
		{
		return -2;
		}
	    else
		{
		/** reset the signal handler to the default for this signal if there's no handler functions left **/
		if(xaCount(list)==0)
		    {
		    signal(signum,SIG_DFL);
		    }
		return 0;
		}
	    }
	}
    return -1;
    }


/*** MTTICKS is a friendly interface to times() so we can get the clock ticks 
 *** since the program started.
 ***/
unsigned long
mtTicks()
    {
    static struct tms t;
    unsigned int x;
    times(&t);
    x = t.tms_utime + t.tms_stime;
    return x - MTASK.FirstTick;
    }


/*** MTREALTICKS is another friendly interface to times() so that we can get
 *** "real time" clock ticks from point a to point b.
 ***/
unsigned long
mtRealTicks()
    {
    static struct tms t;
    return times(&t);
    }


/*** MTLASTTICK is the most recent realticks value as of the last run of the
 *** scheduler.  Will be always valid following the return of a command which
 *** had to wait on other processes.
 ***/
unsigned long
mtLastTick()
    {
    return MTASK.LastTick;
    }


/*** MTFDMAX calculates the max-fd value for a select statement based on an
 *** accumulating value and the value of a to-be-added file descriptor.  See the
 *** select() manual page for more information.
 ***/
int
mtFdMax(int cur_max, int new_fd)
    {
    return (new_fd+1 > cur_max)?(new_fd+1):cur_max;
    }


/*** MTINITIALIZE is used to start the first thread in the process and thus
 *** initialize the MTASK system.  Returns a thread descriptor.  After this call,
 *** there will be one executing thread of control in the process.
 ***/
pThread
mtInitialize(int flags, void (*start_fn)())
    {
    register int i;
    gid_t grouplist[1];

	memset(&MTASK, 0, sizeof(MTASK));

    	/** Initialize the thread table. **/
	MTASK.nThreads = 0;
	for(i=0;i<MAX_THREADS;i++) MTASK.ThreadTable[i] = NULL;

	/** Initialize the system-event-wait table **/
	MTASK.nEvents = 0;
	for(i=0;i<MAX_EVENTS;i++) MTASK.EventWaitTable[i] = NULL;

	/** initialize the ring buffer for pending signals **/
	xrqInit(&MTASK.PendingSignals,4);
	
	/** initialize the hash table for signal handlers **/
	xhInit(&MTASK.SignalHandlers,16,4);

	/** If we are running as root, clear the supplementary groups
	 ** list.  This isn't optimal, but solves the security issue for
	 ** the time being.  Otherwise, threads running as other users
	 ** end up with root's supplementary groups.  When not running as
	 ** root you can't switch to other users anyhow so it isn't an
	 ** issue there.
	 **/
	if (geteuid() == 0 || getuid() == 0) setgroups(0,grouplist);

	/** Create the first thread. **/
	MTASK.CurrentThread = (pThread)nmMalloc(sizeof(Thread));
	if (!MTASK.CurrentThread) return NULL;
	strcpy(MTASK.CurrentThread->Name,"Init");
	MTASK.CurrentThread->EventCkFn = evThread;
	MTASK.CurrentThread->CurPrio = 64;
	MTASK.CurrentThread->BasePrio = 64;
	MTASK.CurrentThread->CntDown = 0;
	MTASK.CurrentThread->Status = THR_S_RUNNABLE;
	MTASK.CurrentThread->Flags = THR_F_STARTING;
	MTASK.CurrentThread->StartFn = start_fn;
	MTASK.CurrentThread->StartParam = NULL;
	MTASK.CurrentThread->SecContext.UserID = geteuid();
	MTASK.CurrentThread->SecContext.GroupID = getegid();
	MTASK.CurrentThread->SecContext.ThrParam = NULL;
	MTASK.CurrentThread->Stack = NULL;
	MTASK.CurrentThread->StackBottom = NULL;
#ifdef USING_VALGRIND
	MTASK.CurrentThread->ValgrindStackID = 0;
#endif
	MTASK.StartUserID = geteuid();
	MTASK.CurUserID = geteuid();
	MTASK.CurGroupID = getegid();

	/** Add it to the table **/
	MTASK.nThreads = 1;
	MTASK.ThreadTable[0] = MTASK.CurrentThread;

	/** Set the MTASK flags **/
	MTASK.MTFlags = (flags & (MT_F_NOYIELD)) | MT_F_ONEPROC;

	/** Initialize the timer. **/
	MTASK.FirstTick = 0;
	MTASK.FirstTick = mtTicks();
	MTASK.TickCnt = 0;

#ifdef _SC_CLK_TCK
	MTASK.TicksPerSec = sysconf(_SC_CLK_TCK);
#else
	MTASK.TicksPerSec = CLK_TCK;
#endif
	
	/** Initialize the thread creation jmp buffer **/
	mtRunStartFn(NULL,0);

	/** Initialize signals **/
	signal(SIGPIPE, mtSigPipe);
	signal(SIGSEGV, mtSigSegv);

	/** Now start the real start function. **/
	MTASK.CurrentThread = NULL;
	mtSched();

    return OK;
    }


/*** MTRUNSTARTFN allocates stack space and starts a new thread's new
 *** function.  This is a bit strange.  We have to have some kind of
 *** platform independent way of creating stack area, so we call this
 *** function recursively to bump the stack ptr to where it should be.
 *** The large local variable eats stack area.  We do the longjmp first
 *** so that the stack is at a known position before doing the count.
 *** Otherwise, the stack offset generated by the repeated recursive
 *** calls will differ depending on what the calling thread is and what
 *** its current function-nest-level is.
 ***/
static jmp_buf r_saved_env;
static pThread r_newthr;
static int r_newidx;
int
mtRunStartFn(pThread new_thr, int idx)
    {

	/** We need these static variables because the automatic **/
	/** variables get clobbered when we do the longjmp **/
        r_newidx = idx + 1;
        r_newthr = new_thr;

        /** if thr is null, we prime the jmp buffer **/
        if (!r_newthr)
            {
 	    if (setjmp(r_saved_env) == 0) return 0;
	    r_mtRunStartFn();
	    }
        else
	    {
	    longjmp(r_saved_env,1);
	    }

    return 0; /* never returns */
    }

int
r_mtRun_Spacer()
    {
    /** I know this issues a compiler warning.  This is here because
     ** it needs to be in order for MTASK to work.  DO NOT OPTIMIZE
     ** THIS MODULE!!!!  The bogus assignment is added to keep gcc -Wall
     ** happy (and silent)....
     **/
    char buf[MT_TASKSEP];
    buf[0] = buf[0];

    /*mprotect((char*)((int)(buf-MAX_STACK+MT_TASKSEP*2+4095) & ~4095), MT_TASKSEP/2, PROT_NONE);*/
    MTASK.CurrentThread->Stack = (unsigned char*)buf;
#ifdef USING_VALGRIND
    MTASK.CurrentThread->ValgrindStackID = VALGRIND_STACK_REGISTER(buf - MAX_STACK + MT_TASKSEP, buf + MT_TASKSEP + 20);
#endif
    if (!MTASK.CurrentThread->StackBottom) MTASK.CurrentThread->StackBottom = (unsigned char*)buf;
    r_newthr->StartFn(r_newthr->StartParam);
    return 0; /* never returns */
    }

int
r_mtRunStartFn()
    {
    /** I know this issues a compiler warning.  This is here because
     ** it needs to be in order for MTASK to work.  DO NOT OPTIMIZE
     ** THIS MODULE!!!!  The bogus assignment is added to keep gcc -Wall
     ** happy.
     **/
    char buf[MAX_STACK];
    buf[0] = buf[0];

    if (r_newidx < 0) return 0;
    if (--r_newidx) r_mtRunStartFn();
    r_mtRunStartFn();
    r_mtRun_Spacer();
    /* thExit(); */
    return 0;	/* should never return */
    }

/*** mtProcessSignals is an internal-only function to process the list of pending signals
 ***   I had to add this, as there's three places to process signals, and I didn't want to put this code
 ***   in all three places (beginning of mtSched, and after an EINTR or EAGAIN on either select())
 ***/
int mtProcessSignals()
    {
    int processed = 0;
    int *sig;
    while((sig = (int*)xrqDequeue(&MTASK.PendingSignals)))
	{
	pXArray list;
	list = (pXArray) xhLookup(&MTASK.SignalHandlers,(char*)sig);
	if(list)
	    {
	    int i;
	    for(i=0;i<xaCount(list);i++)
		{
		void (*start_fn)() = xaGetItem(list,i);
		thCreate(start_fn,0,NULL);
		processed++;
		}
	    }
	}
    return processed;
    }

/*** MTSCHED is the main thread scheduling and blocking i/o processing
 *** piece of the MTASK system.  This routine is internal-only and not
 *** exposed to the "outside world".
 ***/
int
mtSched()
    {
    int num_fds;
    int max_fd;
    int t,tx,tx2;
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    struct timeval tmout;
    register int cnt,i;
    int rval;
    int n_runnable, lowest_cntdn, highest_cntdn, lowest_runnable;
    int n_timerblock;
    pThread lowest_run_thr, locked_thr = NULL;
    int ticks_used;
    pThread thr_sleep_init = NULL; /* if thread just did a thSleep */
    pEventReq event;
    int k = 0;
    int arg,len;
    int x[1];

    	dbg_write(0,"x",1);

	/** Is scheduler locked? **/
	if (MTASK.MTFlags & MT_F_LOCKED) locked_thr = MTASK.CurrentThread;

    	/** If the current thread is valid, do processing for it **/
	ticks_used = (t=mtTicks()) - MTASK.TickCnt;
	tx=mtRealTicks();
	if (MTASK.CurrentThread != NULL)
	    {
	    MTASK.CurrentThread->StackBottom = (unsigned char*)x;
	    /** If no tick and thread exec'able, return now; 0 means 'didnt run' **/
	    /** If caller of mtSched sets status to runnable instead of **/
	    /** executing,then this code will force a scheduler 'round' **/
	    if (MTASK.CurrentThread->Status == THR_S_EXECUTING && ticks_used < MT_TICK_MAX)
	        {
		dbg_write(0,"y",1);
		return 0;
		}

	    /** Bump up the cntdown if thread used time **/
	    /** Don't bump if this thread is sleeping because we'll do that later. **/
	    if (ticks_used > 0 && MTASK.CurrentThread->CntDown >= 0) 
	        {
		MTASK.CurrentThread->CntDown += ticks_used*(MTASK.CurrentThread->CurPrio)/MT_TICK_MAX;
		//if (MTASK.ThreadTable[i]->CntDown > 0) MTASK.ThreadTable[i]->CntDown = 0;
		/*
		if (MTASK.ThreadTable[i]->CntDown > 0)
		    printf("UHH OHHH!!!! -- %i (%s) CntDown == %i\n",i,
			    MTASK.ThreadTable[i]->Name,MTASK.ThreadTable[i]->CntDown);
		*/
		MTASK.TickCnt = t;
		}

	    /** Remember if this thread just init'd a sleep, because if it did, we don't want **/
	    /** to credit time it used before sleeping against its sleep time. **/
	    if (MTASK.CurrentThread->CntDown < 0) thr_sleep_init = MTASK.CurrentThread;

	    /** Do a setjmp() so we can return to caller after scheduling. **/
	    if (setjmp(MTASK.CurrentThread->SavedEnv) != 0) 
	        {
		dbg_write(0,"s",1);
		return 1;
		}
	    if (MTASK.CurrentThread->Status == THR_S_EXECUTING) MTASK.CurrentThread->Status = THR_S_RUNNABLE;
	    MTASK.CurrentThread = NULL;
	    }

	/** Build select() criteria based on system event wait table **/
	/** Also look for EV_T_MT_TIMER events and start building the select() timeout **/
    RETRY_SELECT:
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	max_fd = 1;
	num_fds = 0;
	i = cnt = 0;
	highest_cntdn = 0x80000000;
	n_runnable = 0;
	n_timerblock = 0;
	for(i=cnt=0;cnt<MTASK.nEvents;i++)
	    {
	    if (MTASK.EventWaitTable[i])
	        {
	        cnt++;
		if (locked_thr && locked_thr != MTASK.EventWaitTable[i]->Thr) continue;
	        if (MTASK.EventWaitTable[i]->ObjType == OBJ_T_FD && MTASK.EventWaitTable[i]->Status == EV_S_INPROC)
	            {
		    if (MTASK.EventWaitTable[i]->EventType == EV_T_FINISH &&
		        !(FD_ISSET(((pFile)(MTASK.EventWaitTable[i]->Object))->FD,&exceptfds)))
			{
			MTASK.EventWaitTable[i]->Status = EV_S_COMPLETE;
			MTASK.EventWaitTable[i]->Thr->Status = THR_S_RUNNABLE;
			continue;
			}
		    max_fd = mtFdMax(max_fd, ((pFile)(MTASK.EventWaitTable[i]->Object))->FD);
		    if (MTASK.EventWaitTable[i]->EventType == EV_T_FD_READ)
		        {
		        FD_SET(((pFile)(MTASK.EventWaitTable[i]->Object))->FD,&readfds);
#ifdef MTASK_DEBUG
			if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_READ_SELECTABLE)
			    printf("Added %i to readable FDSET\n",((pFile)(MTASK.EventWaitTable[i]->Object))->FD);
#endif
		        }
		    if (MTASK.EventWaitTable[i]->EventType == EV_T_FD_WRITE || MTASK.EventWaitTable[i]->EventType == EV_T_FD_OPEN)
		        {
		        FD_SET(((pFile)(MTASK.EventWaitTable[i]->Object))->FD,&writefds);
#ifdef MTASK_DEBUG
			if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_WRITE_SELECTABLE)
			    printf("Added %i to writeable FDSET\n",((pFile)(MTASK.EventWaitTable[i]->Object))->FD);
#endif
		        }
		    FD_SET(((pFile)(MTASK.EventWaitTable[i]->Object))->FD,&exceptfds);
#ifdef MTASK_DEBUG
		    if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_ERROR_SELECTABLE)
			printf("Added %i to exception FDSET\n",((pFile)(MTASK.EventWaitTable[i]->Object))->FD);
#endif
		    num_fds++;
		    }
		else if (MTASK.EventWaitTable[i]->EventType == EV_T_MT_TIMER)
		    {
		    k = MTASK.EventWaitTable[i]->TargetTickCnt - tx;
		    if (k < 0) k = 0;
		    k = -(k*64);
		    if (k > highest_cntdn) highest_cntdn = k;
		    if (MTASK.EventWaitTable[i]->Thr->Status == THR_S_BLOCKED) n_timerblock++;
#ifdef MTASK_DEBUG
		    if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_TIMER_SELECTABLE)
			printf("%s is blocked on a timer (k=%i) (cnt=%lu)\n",MTASK.EventWaitTable[i]->Thr->Name,k, MTASK.EventWaitTable[i]->TargetTickCnt);
#endif
		    }
		}
	    }

	/** Determine how to issue the select() by scanning the thread tbl **/
	/** We also credit time used by last thread to any sleeping threads **/
	i=cnt=0;
	for(i=cnt=0;cnt<MTASK.nThreads;i++)
	    {
	    if (MTASK.ThreadTable[i])
	        {
		cnt++;
		if (locked_thr && locked_thr != MTASK.ThreadTable[i]) continue;
		if (MTASK.ThreadTable[i]->Status == THR_S_RUNNABLE)
		    {
		    n_runnable++;
		    if (MTASK.ThreadTable[i]->CntDown < 0)
		        {
			if (MTASK.ThreadTable[i] != thr_sleep_init)
			    {
			    MTASK.ThreadTable[i]->CntDown += ticks_used*MTASK.ThreadTable[i]->CurPrio;
			    if (MTASK.ThreadTable[i]->CntDown > 0) MTASK.ThreadTable[i]->CntDown = 0;
			    }
			if (MTASK.ThreadTable[i]->CntDown > highest_cntdn)
		            {
			    highest_cntdn = (MTASK.ThreadTable[i]->CntDown)*64/(MTASK.ThreadTable[i]->CurPrio);
			    }
			}
		    else
		        {
			/** If threads are runnable and not sleeping, then we select() below with delay=0. **/
			highest_cntdn = 0;
			}
		    }
		}
	    }

	/** Issue the select.  If threads are runnable, delay is 0.  If none runnable (all **/
	/** normally runnable are blocked on i/o or otherwise), delay is either infinite or **/
	/** the correct value to sleep() based on previous thSleep calls made by threads. **/
	if (n_runnable+n_timerblock == 0)
	    {
          REISSUE_SELECT:
	    if(mtProcessSignals()>0)
		{
		return mtSched();
		}
#ifdef MTASK_DEBUG
	    if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_IO_SELECT)
		printf("IO select (no timeout)\n");
#endif
	    rval = select(max_fd, &readfds, &writefds, &exceptfds, NULL);
#ifdef MTASK_DEBUG
	    if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_IO_SELECT)
		printf("IO select done (no timeout) rval=%i errno=%i\n",rval,errno);
#endif
	    if (rval == -1)
		{
		if (errno == EINTR || errno == EAGAIN) goto REISSUE_SELECT;
		perror("MTASK: non-timeout select() failed");
		}
	    }
	else
	    {
	    int ticklen=(1000000/MTASK.TicksPerSec); /** length of a tick in usecs **/
	    if (highest_cntdn == 0x80000000) highest_cntdn = 0;
	    highest_cntdn = -highest_cntdn;
	    /*if (num_fds > 0)*/
	        {
	      REISSUE_SELECT2:
		if(mtProcessSignals()>0)
		    {
		    return mtSched();
		    }
	        tmout.tv_sec = highest_cntdn/(64*MTASK.TicksPerSec);
	        tmout.tv_usec = ((long long) (highest_cntdn - tmout.tv_sec*64*MTASK.TicksPerSec)) * 1000000/(64*MTASK.TicksPerSec);
#ifdef MTASK_DEBUG
		if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_NON_IO_SELECT)
		    {
		    printf("highest_cntdn: %i\n", highest_cntdn);
		    printf("MTASK.TicksPerSec: %i\n", MTASK.TicksPerSec);
		    }
#endif
		/** if we need 4.5 ticks, we need to select for at least 5 to make sure we don't get 4 and 'deadlock' **/
		tmout.tv_usec= ((tmout.tv_usec+(ticklen-1))/ticklen)*ticklen;
		if(tmout.tv_usec>=1000000)
		    {
		    tmout.tv_usec-=1000000;
		    tmout.tv_sec+=1;
		    }
#ifdef MTASK_DEBUG
		if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_NON_IO_SELECT)
		    printf("non-IO select (%i,%i)\n",(int)tmout.tv_sec,(int)tmout.tv_usec);
#endif
	        rval = select(max_fd, &readfds, &writefds, &exceptfds, &tmout);
#ifdef MTASK_DEBUG
		if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_NON_IO_SELECT)
		    printf("non-IO select done (%i,%i) rval=%i errno=%i\n",(int)tmout.tv_sec,(int)tmout.tv_usec,rval,errno);
#endif
	        if (rval == -1)
		    {
		    if (errno == EINTR || errno == EAGAIN) goto REISSUE_SELECT2;
		    perror("MTASK: timeout select() failed");
		    }
		}
	    }

	/** Did the select() delay?  If so, add to sleeping threads. **/
	tx2 = mtRealTicks();
	MTASK.LastTick = tx2;

	if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_SELECT_TICKS_USED)
	    {
	    printf("old ticks: %i\n", tx);
	    printf("current ticks: %i\n", tx2);
	    printf("ticks used: %i\n", tx2-tx);
	    }

	if (n_runnable == 0 && highest_cntdn > 0) t = mtTicks();
	if (tx2 - tx > 0)
	    {
	    if (n_timerblock && highest_cntdn > 0)
	        {
		cnt=i=0;
		while(cnt<n_runnable)
		    {
		    if (MTASK.ThreadTable[i] && MTASK.ThreadTable[i]->Status == THR_S_RUNNABLE)
		        {
			cnt++;
			if (MTASK.ThreadTable[i]->CntDown < 0)
			    {
			    MTASK.ThreadTable[i]->CntDown += (MTASK.ThreadTable[i]->CurPrio*(tx2-tx));
			    if (MTASK.ThreadTable[i]->CntDown > 0) MTASK.ThreadTable[i]->CntDown = 0;
			    }
			}
		    i++;
		    }
		}
	    }
	MTASK.TickCnt = t;

	/** Did any file descriptors "complete"?  If so, pull the event(s) **/
	i = cnt = 0;
	for(i=cnt=0;cnt<MTASK.nEvents;i++)
	    {
	    if (MTASK.EventWaitTable[i])
	        {
		event = MTASK.EventWaitTable[i];
	        cnt++;
		if (locked_thr && locked_thr != event->Thr) continue;
	        if (event->ObjType == OBJ_T_FD)
	            {
		    /** File descriptor become readable? **/
		    if (event->EventType == EV_T_FD_READ)
		        {
		        if (FD_ISSET(((pFile)(event->Object))->FD,&readfds))
			    {
#ifdef MTASK_DEBUG
			    if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_READ_SELECTED)
				printf("Found %i in readable FDSET\n",((pFile)(event->Object))->FD);
#endif
			    FD_CLR(((pFile)(event->Object))->FD,&readfds);
			    dbg_write(0,"r",1);
			    event->Thr->Status = THR_S_RUNNABLE;
			    ((pFile)(event->Object))->Flags &= ~FD_F_RDBLK;
			    event->Status = EV_S_COMPLETE;
			    }
		        }
		
		    /** File descriptor become writable? **/
		    if (event->EventType == EV_T_FD_WRITE || event->EventType == EV_T_FD_OPEN)
		        {
		        if (FD_ISSET(((pFile)(event->Object))->FD,&writefds))
			    {
#ifdef MTASK_DEBUG
			    if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_WRITE_SELECTED)
				printf("Found %i in writeable FDSET\n",((pFile)(event->Object))->FD);
#endif
			    FD_CLR(((pFile)(event->Object))->FD,&writefds);
			    dbg_write(0,"w",1);
			    event->Thr->Status = THR_S_RUNNABLE;
			    ((pFile)(event->Object))->Flags &= ~FD_F_WRBLK;
			    if (((pFile)(event->Object))->Status == FD_S_OPENING) 
			        {
				/** Check getsockopt() to see if successful.
				 ** We interpret getsockopt() error as a success,
				 ** as if the fd is not a socket.  If a socket,
				 ** we check the SO_ERROR value itself.
				 **/
				len = sizeof(arg);
				if (getsockopt(((pFile)(event->Object))->FD, SOL_SOCKET, SO_ERROR, (void*)&arg, &len) < 0 || arg == 0)
				    {
				    ((pFile)(event->Object))->Status = FD_S_OPEN;
				    event->Status = EV_S_COMPLETE;
				    }
				else
				    {
				    ((pFile)(event->Object))->Status = FD_S_ERROR;
				    event->Status = EV_S_ERROR;

				    /** Probably we shouldn't be directly setting
				     ** the errno like this, but I'm gonna go and
				     ** do it anyhow.  This may get us in trouble
				     ** if we use native threads, but (duh!) this
				     ** scheduler won't even be used then!!!
				     **/
				    errno = arg;
				    }
				}
			    else
				{
				event->Status = EV_S_COMPLETE;
				}
			    }
		        }

		    /** Or, did the descriptor get an exception/error? **/
		    if (FD_ISSET(((pFile)(event->Object))->FD,&exceptfds))
			{
#ifdef MTASK_DEBUG
			if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_ERROR_SELECTED)
			    printf("Found %i in exception FDSET\n",((pFile)(event->Object))->FD);
#endif
			dbg_write(0,"e",1);
			event->Thr->Status = THR_S_RUNNABLE;
			event->Status = EV_S_ERROR;
			}
		    }
		else if (event->EventType == EV_T_MT_TIMER)
		    {
		    if (((unsigned int)tx2 - event->TargetTickCnt) < (unsigned int)0x80000000)
		        {
#ifdef MTASK_DEBUG
			if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_TIMER_SELECTED)
			    printf("Timer completed for %s\n",event->Thr->Name);
#endif
			n_timerblock--;
			event->Thr->Status = THR_S_RUNNABLE;
			event->Status = EV_S_COMPLETE;
			}
		    }
		}
	    }

	/** Adjust scheduling count-down values according to priorities **/
	lowest_cntdn = 0x7FFFFFFF;
	lowest_runnable = 0x7FFFFFFF;
	lowest_run_thr = NULL;
	for(i=cnt=0;cnt<MTASK.nThreads;i++)
	    {
	    if (MTASK.ThreadTable[i])
	        {
		cnt++;
		if (MTASK.ThreadTable[i]->CntDown >= 0)
		    {
		    if (MTASK.ThreadTable[i]->CntDown < lowest_cntdn)
		        {
			lowest_cntdn = MTASK.ThreadTable[i]->CntDown;
			}
		    if (MTASK.ThreadTable[i]->Status == THR_S_RUNNABLE &&
		        MTASK.ThreadTable[i]->CntDown < lowest_runnable)
			{
			lowest_runnable = MTASK.ThreadTable[i]->CntDown;
			lowest_run_thr = MTASK.ThreadTable[i];
			k = i;
			}
		    }
		}
	    }

	/** Ok, now subtract the smallest cntdn from all positive threads. **/
	if (lowest_cntdn > 0 && lowest_cntdn != 0x7FFFFFFF)
	    {
	    for(i=cnt=0;cnt<MTASK.nThreads;i++)
	        {
		if (MTASK.ThreadTable[i])
		    {
		    cnt++;
		    if (MTASK.ThreadTable[i]->CntDown >=0) MTASK.ThreadTable[i]->CntDown -= lowest_cntdn;
		    }
		}
	    }

	/** Didn't quite sleep long enough? <yuck> **/
	if (!lowest_run_thr && n_timerblock)
	    {
	    tx = tx2;
	    goto RETRY_SELECT;
	    }

	/** Detect a deadlock, exit the program for now. **/
	if (!lowest_run_thr) 
	    {
	    puts("MTASK: Failure: deadlock occurred, aborting...");
            /* Leave a stack trace behind, in case it is useful to someone */
            kill( getpid() , SIGABRT ) ;
	    exit(1);
	    }

	/** If locked, we need to continue with that thread. **/
	if (locked_thr) lowest_run_thr = locked_thr;

#if 0
	if (k==0) dbg_write(0,"0",1);
	if (k==1) dbg_write(0,"1",1);
#endif

	/** Check for any waits on this thread. **/
	for(i=0;i<MTASK.nEvents;i++) if (MTASK.EventWaitTable[i]->Object == lowest_run_thr)
	    {
	    switch (MTASK.EventWaitTable[i]->EventType)
	        {
		case EV_T_THR_START:
		    if (lowest_run_thr->Flags & THR_F_STARTING)
		        {
			MTASK.EventWaitTable[i]->Status = EV_S_COMPLETE;
			MTASK.EventWaitTable[i]->Thr->Status = THR_S_RUNNABLE;
			}
		    break;

		case EV_T_THR_TIME:
		    MTASK.EventWaitTable[i]->Status = EV_S_COMPLETE;
		    MTASK.EventWaitTable[i]->Thr->Status = THR_S_RUNNABLE;
		    break;
		}
	    }

	/** Start the selected thread. **/
	MTASK.CurrentThread = lowest_run_thr;
	lowest_run_thr->Status = THR_S_EXECUTING;

	/** Switch to that thread's UID if different from current. **/
	if (lowest_run_thr->SecContext.UserID != MTASK.CurUserID) 
	    {
	    if (MTASK.CurUserID != 0) seteuid(0);
	    if (lowest_run_thr->SecContext.UserID != 0) seteuid(lowest_run_thr->SecContext.UserID);
	    MTASK.CurUserID = lowest_run_thr->SecContext.UserID;
	    }
	if (lowest_run_thr->SecContext.GroupID != MTASK.CurGroupID)
	    {
	    if (MTASK.CurUserID != 0) seteuid(0);
	    if (lowest_run_thr->SecContext.GroupID != 0) setegid(lowest_run_thr->SecContext.GroupID);
	    if (MTASK.CurUserID != 0) seteuid(MTASK.CurUserID);
	    MTASK.CurGroupID = lowest_run_thr->SecContext.GroupID;
	    }

	/** Jump into the thread... **/
	if (lowest_run_thr->Flags & THR_F_STARTING)
	    {
	    lowest_run_thr->Flags &= ~THR_F_STARTING;
	    dbg_write(0,"c",1);
	    mtRunStartFn(lowest_run_thr,k);
	    }
	else
	    {
	    dbg_write(0,"l",1);
	    longjmp(lowest_run_thr->SavedEnv,1);
	    }

    return 1;
    }


/*** THCREATE creates a new thread and starts it on a given start function with
 *** a starting priority (0 for default) and a starting parameter, which can be
 *** an integer, pointer to structure, etc.
 ***/
pThread
thCreate(void (*start_fn)(), int priority, void* start_param)
    {
    pThread thr;
    int i;

    	if (MTASK.nThreads >= MAX_THREADS)
	    {
	    printf("mtask: Thread Table Limit (%d) exceeded!\n",MAX_THREADS);
	    return NULL;
	    }

    	/** Create the new thread descriptor **/
	thr = (pThread)nmMalloc(sizeof(Thread));
	if (!thr) return NULL;

	/** Fill in the structure. **/
	thr->EventCkFn = evThread;
	strcpy(thr->Name,"<starting>");
	thr->CurPrio = priority?priority:64;
	thr->BasePrio = thr->CurPrio;
	thr->CntDown = 0;
	thr->Status = THR_S_RUNNABLE;
	thr->Flags = THR_F_STARTING;
	thr->StartFn = start_fn;
	thr->StartParam = start_param;
	thr->SecContext.UserID = MTASK.CurUserID;
	thr->SecContext.GroupID = MTASK.CurGroupID;
	thr->Stack = NULL;
	thr->StackBottom = NULL;
#ifdef USING_VALGRIND
	thr->ValgrindStackID = 0;
#endif
	/** copy the thread param from the current thread, if there is one
	      this allows the signal handler, which will sometimes get called while
	      the scheduler is in a select() call to spawn a new thread **/
	if(MTASK.CurrentThread)
	    thr->SecContext.ThrParam = MTASK.CurrentThread->SecContext.ThrParam;
	else
	    thr->SecContext.ThrParam = NULL;

	/** Add to the thread table. **/
	for(i=0;i<MAX_THREADS;i++)
	    {
	    if (!MTASK.ThreadTable[i])
	        {
		MTASK.ThreadTable[i] = thr;
		break;
		}
	    }
	MTASK.nThreads++;

	/** Call the scheduler to start the thread. **/
	mtSched();

    return thr;
    }


/*** THYIELD invokes the scheduler and forces a yield-type
 *** operation by setting the thread's status to runnable instead
 *** of executing first.
 ***/
int
thYield()
    {

	dbg_write(0,"Y",1);

    	/** Change thread's status. **/
	MTASK.CurrentThread->Status = THR_S_RUNNABLE;

	/** Call the scheduler **/
	mtSched();

    return OK;
    }


/*** THEXIT exits and destroys the current thread and then calls 
 *** the scheduler to invoke the next thread.
 ***/
int
thExit()
    {
    register int i;

	dbg_write(0,"E",1);

    	/** Remove thread from thread table **/
	for(i=0;i<MAX_THREADS;i++) 
	    {
	    if (MTASK.ThreadTable[i] == MTASK.CurrentThread)
	        {
		MTASK.ThreadTable[i] = NULL;
		MTASK.nThreads--;
		break;
		}
	    }

	/** Any thread waiting on this thread? **/
	for(i=0;i<MTASK.nEvents;i++) if (MTASK.EventWaitTable[i]->Object == MTASK.CurrentThread)
	    {
	    if (MTASK.EventWaitTable[i]->EventType == EV_T_THR_EXIT)
	        {
	        MTASK.EventWaitTable[i]->Status = EV_S_COMPLETE;
		}
	    else
	        {
	        MTASK.EventWaitTable[i]->Status = EV_S_ERROR;
		}
	    MTASK.EventWaitTable[i]->Thr->Status = THR_S_RUNNABLE;
	    }

#ifdef USING_VALGRIND
	VALGRIND_STACK_DEREGISTER(MTASK.CurrentThread->ValgrindStackID);
#endif

	/** Destroy the thread's descriptor **/
	nmFree(MTASK.CurrentThread,sizeof(Thread));
	MTASK.CurrentThread = NULL;

	/** No more threads? **/
	if (MTASK.nThreads == 0) exit(0);

	/** Call scheduler **/
	MTASK.MTFlags &= ~MT_F_LOCKED;
	mtSched();

    return OK; /* though we never return - mtSched doesn't do a setjmp... */
    }


/*** THKILL terminates another specified thread.  If a thread tries
 *** to "commit suicide" (i.e., thKill(NULL)), this function transfers
 *** control to thExit().
 ***/
int
thKill(pThread thr)
    {
    register int i;

    	/** Param check **/
	if (!thr) thr = MTASK.CurrentThread;

	/** Current thread?  If so, do thExit. **/
	if (thr == MTASK.CurrentThread) thExit();

	/** Thread has any waits in system event wait table? **/
	for(i=0;i<MTASK.nEvents;i++) if (MTASK.EventWaitTable[i]->Thr == thr)
	    {
	    nmFree(evRemoveIdx(i),sizeof(EventReq));
	    i--;
	    }

	/** Any thread waiting on this thread? **/
	for(i=0;i<MTASK.nEvents;i++) if (MTASK.EventWaitTable[i]->Object == thr)
	    {
	    if (MTASK.EventWaitTable[i]->EventType == EV_T_THR_EXIT)
	        {
	        MTASK.EventWaitTable[i]->Status = EV_S_COMPLETE;
		}
	    else
	        {
	        MTASK.EventWaitTable[i]->Status = EV_S_ERROR;
		}
	    MTASK.EventWaitTable[i]->Thr->Status = THR_S_RUNNABLE;
	    }

	/** Remove from thread table. **/
	for(i=0;i<MAX_THREADS;i++) if (MTASK.ThreadTable[i] == thr)
	    {
	    MTASK.ThreadTable[i] = NULL;
	    MTASK.nThreads--;
	    break;
	    }

#ifdef USING_VALGRIND
	VALGRIND_STACK_DEREGISTER(thr->ValgrindStackID);
#endif

	/** Free the structure. **/
	nmFree(thr, sizeof(Thread));

	/** Schedule next thread **/
	mtSched();

    return OK;
    }


/*** THWAIT causes the current thread to wait until a certain event
 *** occurs.  Could be file readable, thread started, etc.  The arg count
 *** parameter means different things for different event types.  This
 *** is a single-event optimized version of thMultiWait.  It was kept
 *** separate for a reason <grin>....
 ***/
int
thWait(pMTObject obj, int obj_type, int event_type, int arg_count)
    {
    int code, sched_called = 0;
    pEventReq event;

    	/** Is this just a timer event?  Call sleep if so. **/
	if (event_type == EV_T_MT_TIMER)
	    {
	    thSleep(arg_count);
	    return 0;
	    }

    	/** Check the thing first to see if we need to block **/
	code = obj->EventCkFn(event_type,obj);

	/** Loop until we get proper yield/event **/
	while(1)
	    {
	    /** Invalid event wait? **/
	    if (code == -1) return -1;

	    /** If MTASK is set to always-yield then do that now. **/
	    if (code == 1 && !(MTASK.MTFlags & MT_F_NOYIELD) && !sched_called)
	        {
	        if (mtSched() != 0)
	            {
		    sched_called = 1;
		    if (obj_type == OBJ_T_THREAD)
			{
			code = 1;
			break;
			}
		    if (obj_type == OBJ_T_SEM) break;
		    code = obj->EventCkFn(event_type,obj);
		    }
		}
	    if (code == 1) break;

	    /** If this is going to block, set up an event structure **/
	    if (code == 0)
	        {
		event = (pEventReq)nmMalloc(sizeof(EventReq));
		if (!event) return -1;
		event->Thr = MTASK.CurrentThread;
		event->Object = (void*)obj;
		event->ObjType = obj_type;
		event->EventType = event_type;
		event->Status = EV_S_INPROC;
		event->ReqLen = arg_count;
		event->ReqFlags = 0;
		event->NextPeer = event;
		event->TableIdx = evAdd(event);
		if (event->TableIdx == -1)
		    {
		    nmFree(event, sizeof(EventReq));
		    return -1;
		    }
		MTASK.CurrentThread->Status = THR_S_BLOCKED;
		mtSched();
		evRemoveIdx(event->TableIdx);
		sched_called = 1;
		code = event->Status;
		nmFree(event, sizeof(EventReq));
		}
	    }

    return (code==1)?0:(-1);
    }


/*** THMULTIWAIT waits on multiple events until at least one of the
 *** events completes.  The completed event is indicated by the statuses
 *** assigned to each event req passed.
 ***/
int
thMultiWait(int event_cnt, pEventReq event_req[])
    {
    register int i;
    int code = 0;
    int sched_called = 0;

    	/** Check the thing first to see if we need to block **/
	for(i=0;i<event_cnt;i++)
	    {
	    if (event_req[i]->EventType == EV_T_MT_TIMER)
	        {
		code = (event_req[i]->ReqLen == 0)?1:0;
		}
	    else
	        {
	        event_req[i]->Status = EV_S_INPROC;
	        code = PMTOBJECT(event_req[i]->Object)->EventCkFn(event_req[i]->EventType,event_req[i]->Object);
		}
	    if (code == -1) event_req[i]->Status = EV_S_ERROR;
	    if (code == 1) event_req[i]->Status = EV_S_COMPLETE;
	    if (code == -1 || code == 1) break;
	    }

	/** Loop until we get proper yield/event **/
	while(1)
	    {
	    /** Invalid event wait? **/
	    if (code == -1) return -1;

	    /** If MTASK is set to always-yield then do that now. **/
	    if (code == 1 && !(MTASK.MTFlags & MT_F_NOYIELD) && !sched_called)
	        {
	        if (mtSched() != 0)
	            {
		    sched_called = 1;
		    for(i=0;i<event_cnt;i++)
	    	        {
			if (event_req[i]->ObjType == OBJ_T_THREAD)
			    {
			    code = 1;
			    break;
			    }
			if (event_req[i]->ObjType == OBJ_T_SEM) break;
	    		if (event_req[i]->EventType == EV_T_MT_TIMER) break;
			code = PMTOBJECT(event_req[i]->Object)->EventCkFn(event_req[i]->EventType,event_req[i]->Object);
			if (code == 1) event_req[i]->Status = EV_S_COMPLETE;
			else if (code == -1) event_req[i]->Status = EV_S_ERROR;
			else event_req[i]->Status = EV_S_INPROC;
	    		if (code == -1 || code == 1) break;
	    		}
		    }
		}
	    if (code == 1) break;

	    /** If this is going to block, set up an event structure **/
	    if (code == 0)
	        {
		for(i=0;i<event_cnt;i++)
		    {
		    event_req[i]->Thr = MTASK.CurrentThread;
		    event_req[i]->Status = EV_S_INPROC;
		    if (i>0) 
		        {
			event_req[i]->NextPeer = event_req[i-1];
			}
		    else
		        {
			event_req[i]->NextPeer = event_req[event_cnt-1];
			}
		    if (event_req[i]->EventType == EV_T_MT_TIMER)
		        {
			event_req[i]->TargetTickCnt = mtRealTicks() + (event_req[i]->ReqLen)*MTASK.TicksPerSec/1000;
			}
		    event_req[i]->TableIdx = evAdd(event_req[i]);
		    if (event_req[i]->TableIdx == -1)
		        {
		        nmFree(event_req[i], sizeof(EventReq));
		        return -1;
		        }
		    }
		MTASK.CurrentThread->Status = THR_S_BLOCKED;
		mtSched();
		for(i=0;i<event_cnt;i++) 
		    {
		    evRemoveIdx(event_req[i]->TableIdx);
		    if (event_req[i]->Status == EV_S_ERROR) code = -1;
		    if (event_req[i]->Status == EV_S_COMPLETE) code = 1;
		    }
		sched_called = 1;
		}
	    }

    return OK;
    }


/*** THGETNAME: returns the name of the current thread or some other
 *** thread.  If thr is NULL, it means current thread.
 ***/
char*
thGetName(pThread thr)
    {
    if (!thr) thr=MTASK.CurrentThread;
    return thr->Name;
    }


/*** THCURRENT: returns the current thread.
 ***/
pThread
thCurrent()
    {
    return MTASK.CurrentThread;
    }


/*** THSETNAME: changes the name of the current thread or some other
 *** thread.  If thr is NULL, it means current thread.
 ***/
int
thSetName(pThread thr, const char* name)
    {
    if (!thr) thr=MTASK.CurrentThread;
    strncpy(thr->Name, name, STR_LEN-1);
    thr->Name[STR_LEN-1] = 0;
    return 0;
    }


/*** THGETTHREADLIST: gets a process listing of all threads in the
 *** thread table and places information about them into a set of 
 *** arrays passed in by the caller.  Returns number of threads.
 *** The char* [] array should be an array of pointers, which will
 *** be set upon return, NOT COPIED TO.
 ***/
int
thGetThreadList(int max_cnt, int ids[], char* names[], int stati[], int flags[])
    {
    register int i,cnt;

	/** Don't return any more than is in the table. Silly. **/
    	if (max_cnt > MTASK.nThreads) max_cnt = MTASK.nThreads;

	/** Scan the table and build the arrays. **/
    	for(i=cnt=0;cnt<max_cnt;i++) if (MTASK.ThreadTable[i])
	    {
	    ids[cnt] = i;
	    names[cnt] = MTASK.ThreadTable[i]->Name;
	    stati[cnt] = MTASK.ThreadTable[i]->Status;
	    flags[cnt] = MTASK.ThreadTable[i]->Flags;
	    cnt++;
	    }

    return MTASK.nThreads;
    }


/*** THGETBYNAME locates a thread by looking up its name.  The
 *** thread descriptor is returned.
 ***/
pThread
thGetByName(const char* name)
    {
    pThread thr = NULL;
    register int i,cnt;

    	/** Search for the thread. **/
	for(i=cnt=0;cnt<MTASK.nThreads;i++) if (MTASK.ThreadTable[i])
	    {
	    if (!strcmp(name,MTASK.ThreadTable[i]->Name))
	        {
		thr = MTASK.ThreadTable[i];
		break;
		}
	    cnt++;
	    }

    return thr;
    }


/*** THGETPRIO gets the priority for a given thread.  If thr is
 *** NULL, returns the priority for the current thread.
 ***/
int
thGetPrio(pThread thr)
    {
    if (!thr) thr = MTASK.CurrentThread;
    return thr->CurPrio;
    }


/*** THSETPRIO sets the priority for the thread.  If thr is NULL,
 *** then this sets the current thread's priority.
 ***/
int
thSetPrio(pThread thr, int prio)
    {
    int old_prio;
    if (!thr) thr=MTASK.CurrentThread;
    old_prio = thr->CurPrio;
    thr->CurPrio = prio;
    return old_prio;
    }


/*** THLOCK locks the scheduler and prevents other threads
 *** from running.
 ***/
int
thLock()
    {
    MTASK.MTFlags |= MT_F_LOCKED;
    return 0;
    }


/*** THUNLOCK unlocks the scheduler and allows context switches
 *** to other threads to occur.
 ***/
int
thUnlock()
    {
    MTASK.MTFlags &= ~MT_F_LOCKED;
    return 0;
    }



/*** THSLEEP causes the current thread to suspend execution for approximately
 *** the number of MILLISECONDS (1/1000th seconds) specified.
 ***/
int
thSleep(int msec)
    {
#if 0
    int ticks;
#endif
    EventReq e;
    pEventReq e_array[1];

#if 0
	/** How many ticks? **/
	ticks = msec/(1000/MTASK.TicksPerSec);

    	/** Set thread's countdown so scheduler will wait to re-sched **/
	MTASK.CurrentThread->CntDown = -(ticks * MTASK.CurrentThread->CurPrio);
#endif

	e_array[0] = &e;
	e.Object = NULL;
	e.ObjType = OBJ_T_MTASK;
	e.EventType = EV_T_MT_TIMER;
	e.ReqLen = msec;
	thMultiWait(1,e_array);
#if 00
	/** Now we're no longer executing... **/
	MTASK.CurrentThread->Status = THR_S_RUNNABLE;
	mtSched();
#endif

    return 0;
    }



/*** THSETUSERID sets the user id of the given thread.  In order to set the
 *** id, the caller must have ID zero (root).
 ***/
int
thSetUserID(pThread thr, int new_uid)
    {

	/** Setting current thread? **/
	if (!thr) thr = MTASK.CurrentThread;

	/** Verify permissions **/
	if (MTASK.CurrentThread->SecContext.UserID != 0) return -1;

	/** Switch to it. **/
	if (thr->SecContext.UserID != new_uid)
	    {
	    seteuid(new_uid);
	    MTASK.CurUserID = new_uid;
	    }

	/** Set this ID **/
	thr->SecContext.UserID = new_uid;

    return 0;
    }



/*** THGETUSERID gets the user id of the given thread, or current thread
 *** if thr is NULL.
 ***/
int
thGetUserID(pThread thr)
    {

	/** Checking current thread? **/
	if (!thr) thr = MTASK.CurrentThread;

    return thr->SecContext.UserID;
    }


/*** THSETGROUPID sets the group id of the given thread, or current thread
 *** if thr is null.
 ***/
int
thSetGroupID(pThread thr, int new_gid)
    {

	/** Setting current thread? **/
	if (!thr) thr = MTASK.CurrentThread;

	/** Verify permissions **/
	if (MTASK.CurrentThread->SecContext.UserID != 0) return -1;

	/** Switch to it. **/
	if (MTASK.CurGroupID != new_gid)
	    {
	    setegid(new_gid);
	    MTASK.CurGroupID = new_gid;
	    }

	/** Set this ID **/
	thr->SecContext.GroupID = new_gid;

    return 0;
    }


/*** THGETGROUPID returns the current group id or the group id of the given
 *** thread if not null.
 ***/
int
thGetGroupID(pThread thr)
    {

	/** Checking current thread? **/
	if (!thr) thr = MTASK.CurrentThread;

    return thr->SecContext.GroupID;
    }


/*** THSETSECCONTEXT sets the overall security context of a thread to one
 *** provided by the caller.  The context provided must have been obtained
 *** by thGetSecContext() on another thread.  Callers must not directly
 *** modify the security context structure.  Unlike the thSetUserID() call,
 *** in this case the caller need not have root (0) permission.
 ***/
int
thSetSecContext(pThread thr, pMTSecContext context)
    {

	/** Current thread? **/
	if (!thr) thr = MTASK.CurrentThread;

	/** Update thread context **/
	thr->SecContext.UserID = context->UserID;
	thr->SecContext.GroupID = context->GroupID;
	thr->SecContext.ThrParam = context->ThrParam;

	/** Update current run settings if thread is current thread **/
	if (thr == MTASK.CurrentThread)
	    {
	    if (thr->SecContext.UserID != MTASK.CurUserID)
		{
		if (MTASK.CurUserID) seteuid(0);
		if (thr->SecContext.UserID) seteuid(thr->SecContext.UserID);
		MTASK.CurUserID = thr->SecContext.UserID;
		}
	    if (thr->SecContext.GroupID != MTASK.CurGroupID)
		{
		if (MTASK.CurGroupID) setegid(0);
		if (thr->SecContext.GroupID) setegid(thr->SecContext.GroupID);
		MTASK.CurGroupID = thr->SecContext.GroupID;
		}
	    }

    return 0;
    }


/*** THGETSECCONTEXT gets the overall security context of a thread, saving
 *** it in a MTSecContext structure allocated by the caller.  This is usually
 *** done to save the security context of a calling thread, so a new thread
 *** under that security context can be later created as the result of a
 *** totally unrelated event or occurrence (asynchronous from the original
 *** thread's execution).
 ***/
int
thGetSecContext(pThread thr, pMTSecContext context)
    {

	/** Current thread? **/
	if (!thr) thr = MTASK.CurrentThread;

	/** Save it **/
	context->UserID = thr->SecContext.UserID;
	context->GroupID = thr->SecContext.GroupID;
	context->ThrParam = thr->SecContext.ThrParam;

    return 0;
    }


/*** THSETPARAM sets the generic parameter for the specified thread.  This
 *** parameter is a void ptr, and can be used for authentication/session
 *** management.
 ***/
int
thSetParam(pThread thr, const char* name, void* param)
    {
#if 0
    int i;
#endif

    	if (!thr) thr=MTASK.CurrentThread;
#if 0
	/** Find the param or find an empty slot **/
	i=0;
	while(i<16 && thr->ThrParam[i] && strcmp(thr->ThrParam[i]->Name,name)) i++;
	if (i == 16) return -1;

	/** Alloc if need be, and set the new param value. **/
	if (!thr->ThrParam[i])
	    {
	    thr->ThrParam[i] = (pThrExt)nmMalloc(sizeof(ThrExt));
	    strcpy(thr->ThrParam[i]->Name,name);
	    }
	thr->ThrParam[i]->Param = param;
#else
	thr->SecContext.ThrParam = param;
#endif

    return 0;
    }



/*** THGETPARAM returns the specified thread's generic parameter.
 ***/
void*
thGetParam(pThread thr, const char* name)
    {
#if 0
    int i;
#endif

    	if (!thr) thr=MTASK.CurrentThread;
#if 0
	/** Find the param **/
	i=0;
	while(i<16 && (!thr->ThrParam[i] || strcmp(thr->ThrParam[i]->Name,name))) i++;
	if (i == 16) return -1;

    return thr->ThrParam[i]->Param;
#else
    return thr->SecContext.ThrParam;
#endif
    }


/*** THSETFLAGS sets optional flags for the thread.
 ***/
int
thSetFlags(pThread thr, int flags)
    {
    if (!thr) thr=MTASK.CurrentThread;
    thr->Flags |= flags;
    return 0;
    }


/*** THCLEARFLAGS removes optional flags for the thread.
 ***/
int
thClearFlags(pThread thr, int flags)
    {
    if (!thr) thr=MTASK.CurrentThread;
    thr->Flags &= ~flags;
    return 0;
    }



/*** THEXCESSIVERECURSION - check to see if we have used up too much
 *** stack space (still in current stack, but exceeding the allowable
 *** limit minus a safety cushion).
 ***/
int
thExcessiveRecursion()
    {
    unsigned char buf[1];
    return (MTASK.CurrentThread->Stack - buf > MT_STACK_HIGHWATER);
    }


/*** FDSETOPTIONS sets options on an open file descriptor.  These options
 *** are FD_UF_xxx in mtask.h.
 ***/
int
fdSetOptions(pFile filedesc, int options)
    {
    int old_options;
    int arg;

	old_options = filedesc->Flags;
	filedesc->Flags |= (options & (FD_UF_RDBUF | FD_UF_WRBUF | FD_UF_GZIP | FD_UF_BLOCKINGIO));

	/** set blocking io? **/
	if (!(old_options & FD_UF_BLOCKINGIO) && (options & FD_UF_BLOCKINGIO))
	    {
	    arg=0;
	    ioctl(filedesc->FD,FIONBIO,&arg);
	    }
    
#ifdef HAVE_LIBZ
	if ((old_options & FD_UF_WRBUF) && (options & FD_UF_GZIP))
	    {
	    if (filedesc->WrCacheBuf && filedesc->WrCacheLen > 0)
		{
		filedesc->Flags &= ~(FD_UF_WRBUF | FD_UF_GZIP);
		fdWrite(filedesc, filedesc->WrCacheBuf, filedesc->WrCacheLen, 0, FD_U_PACKET);
		filedesc->WrCacheLen = 0;
		filedesc->WrCachePtr = filedesc->WrCacheBuf;
		filedesc->Flags |= (FD_UF_WRBUF | FD_UF_GZIP);
		}
	    }

	if ( (options & FD_UF_GZIP) && !(old_options & FD_UF_GZIP) )
	    {
	    filedesc->GzFile = gzdopen(dup(filedesc->FD), (filedesc->Flags & FD_F_WR ? "wb" : "rb"));
	    }
#endif
    return 0;
    }


/*** FDUNSETOPTIONS resets options on an open file descriptor.  These options
 *** are FD_UF_xxx in mtask.h.
 ***/
int
fdUnSetOptions(pFile filedesc, int options)
    {
    int old_options;
    int arg;

    	old_options = filedesc->Flags;
        filedesc->Flags &= ~(options & (FD_UF_RDBUF | FD_UF_WRBUF | FD_UF_GZIP | FD_UF_BLOCKINGIO));

	/** set nonblocking io? **/
	if ((old_options & FD_UF_BLOCKINGIO) && !(options & FD_UF_BLOCKINGIO))
	    {
	    arg=1;
	    ioctl(filedesc->FD,FIONBIO,&arg);
	    }
    	
	/** Make sure we flush the write-cache. **/
	if ((old_options & FD_UF_WRBUF) && (options & FD_UF_WRBUF))
	    {
	    if (filedesc->WrCacheBuf && filedesc->WrCacheLen > 0)
	        {
		fdWrite(filedesc, filedesc->WrCacheBuf, filedesc->WrCacheLen, 0, FD_U_PACKET);
		}
	    }

#ifdef HAVE_LIBZ
        /** Make sure and close the gzip part if the flag was set but is not currently. **/
        if ((old_options & FD_UF_GZIP) && !(options & FD_UF_GZIP))
            {
                gzclose(filedesc->GzFile);
                filedesc->GzFile = NULL;
            }

#endif
    return 0;
    }


/*** FDPIPE creates to "pFile" descriptors which are an mtask-level (not
 *** kernel level) "pipe".  That is, what is written to one can be read from
 *** the other.  The descriptors are symmetric - no special features are
 *** present on one but not on the other.  Return: 0 on success, -1 on fail.
 ***/
int
fdPipe(pFile *filedesc1, pFile *filedesc2)
    {

	/** Allocate the two descriptors **/
	*filedesc1 = NULL;
	*filedesc2 = NULL;
	*filedesc1 = (pFile)nmMalloc(sizeof(File));
	if (!*filedesc1) goto fdpipe_error;
	memset(*filedesc1, 0, sizeof(File));
	*filedesc2 = (pFile)nmMalloc(sizeof(File));
	if (!*filedesc2) goto fdpipe_error;
	memset(*filedesc2, 0, sizeof(File));

	(*filedesc1)->EventCkFn = (*filedesc2)->EventCkFn = evPipe;
	(*filedesc1)->Flags = (*filedesc2)->Flags = FD_F_PIPE;
	(*filedesc1)->ErrCode = (*filedesc2)->ErrCode = 0;
	(*filedesc1)->Status = (*filedesc2)->Status = FD_S_OPEN;
	(*filedesc1)->FD = (*filedesc2)->FD = -1;
	(*filedesc1)->UnReadLen = (*filedesc2)->UnReadLen = 0;
	(*filedesc1)->WrCacheBuf = (*filedesc2)->WrCacheBuf = NULL;
	(*filedesc1)->RdCacheBuf = (*filedesc2)->RdCacheBuf = NULL;
#ifdef HAVE_LIBZ
	(*filedesc1)->GzFile = (*filedesc2)->GzFile = NULL;
#endif
	(*filedesc1)->PrintfBuf = (*filedesc2)->PrintfBuf = NULL;

	/** Allocate the pipe memory buffers **/
	(*filedesc1)->PipeBuf = nmSysMalloc(FD_PIPE_BUFSIZ);
	(*filedesc2)->PipeBuf = nmSysMalloc(FD_PIPE_BUFSIZ);
	if (!(*filedesc1)->PipeBuf || !(*filedesc2)->PipeBuf) goto fdpipe_error;

	/** Set up pipe data **/
	(*filedesc1)->PipeBufSize = (*filedesc2)->PipeBufSize = FD_PIPE_BUFSIZ;
	(*filedesc1)->PipeBufHead = (*filedesc2)->PipeBufHead = 0;
	(*filedesc1)->PipeBufTail = (*filedesc2)->PipeBufTail = 0;
	(*filedesc1)->OtherFD = (*filedesc2);
	(*filedesc2)->OtherFD = (*filedesc1);

	return 0;

    fdpipe_error:
	if ((*filedesc1)->PipeBuf) nmSysFree((*filedesc1)->PipeBuf);
	if ((*filedesc2)->PipeBuf) nmSysFree((*filedesc2)->PipeBuf);
	if ((*filedesc1)) nmFree((*filedesc1), sizeof(File));
	if ((*filedesc2)) nmFree((*filedesc2), sizeof(File));
	(*filedesc1) = NULL;
	(*filedesc2) = NULL;

    return -1;
    }


/*** FDOPEN opens a new file or device.  Similar to UNIX open() call.
 ***/
pFile
fdOpen(const char* filename, int mode, int create_mode)
    {
    pFile new_fd;
    int fd,arg;

    	/** Open the file. **/
    	fd = open(filename, mode, create_mode);
    	if (fd FAIL) return NULL;

    	/** Create the descriptor structure **/
    	new_fd = (pFile)nmMalloc(sizeof(File));
    	if (!new_fd)
    	    {
    	    close(fd);
    	    return NULL;
    	    }
        new_fd->EventCkFn = evFile;
        new_fd->FD = fd;
        new_fd->Status = FD_S_OPEN;
        new_fd->Flags = (((mode&O_RDONLY)||(mode&O_RDWR))?FD_F_RD:0) |
        		(((mode&O_WRONLY)||(mode&O_RDWR))?FD_F_WR:0);
	new_fd->ErrCode = 0;
	new_fd->UnReadLen = 0;
	new_fd->WrCacheBuf = NULL;
	new_fd->RdCacheBuf = NULL;
#ifdef HAVE_LIBZ
	new_fd->GzFile = NULL;
#endif
	new_fd->PrintfBuf = NULL;

	/** Set nonblocking mode **/
	arg=1;
	ioctl(fd,FIONBIO,&arg);

    return new_fd;
    }


/*** FDOPENFD opens an existing open UNIX file descriptor for use in
 *** the MTASK module.  Have to re-specify the mode so MTASK "knows"
 *** how it was originally open.
 ***/
pFile
fdOpenFD(int fd, int mode)
    {
    pFile new_fd;
    int arg;

    	/** Create the descriptor structure **/
    	new_fd = (pFile)nmMalloc(sizeof(File));
    	if (!new_fd)
    	    {
    	    return NULL;
    	    }
        new_fd->EventCkFn = evFile;
        new_fd->FD = fd;
        new_fd->Status = FD_S_OPEN;
        new_fd->Flags = (((mode&O_RDONLY)||(mode&O_RDWR))?FD_F_RD:0) |
        		(((mode&O_WRONLY)||(mode&O_RDWR))?FD_F_WR:0);
	new_fd->ErrCode = 0;
	new_fd->UnReadLen = 0;
	new_fd->WrCacheBuf = NULL;
	new_fd->RdCacheBuf = NULL;
#ifdef HAVE_LIBZ
	new_fd->GzFile = NULL;
#endif
	new_fd->PrintfBuf = NULL;

	/** Set nonblocking mode **/
	arg=1;
	ioctl(fd,FIONBIO,&arg);

    return new_fd;
    }


/*** FD_INTERNAL_READPKT is an internal-only routine used to read a whole
 *** buffer, not stopping at part of it.
 ***/
int
fd_internal_ReadPkt(pFile filedesc, char* buffer, int maxlen, int offset, int flags)
    {
    int rcnt = 0, cnt=0;

    	/** Repeatedly try until we get it all **/
	while(cnt < maxlen)
	    {
	    rcnt = fdRead(filedesc, buffer + cnt, maxlen - cnt, offset, flags);
	    if (rcnt <= 0) return -1;
	    cnt += rcnt;
	    if (flags & FD_U_SEEK) offset += rcnt;
	    }

    return cnt;
    }


/*** FDREAD reads from a file descriptor.  The flags can contain the
 *** options FD_U_NOBLOCK, FD_U_SEEK.  If FD_U_SEEK is not specified, then
 *** the seek offset is ignored.  FD_U_PACKET means that we should keep on
 *** reading until error or until whole buffer is filled.
 ***/
int
fdRead(pFile filedesc, char* buffer, int maxlen, int offset, int flags)
    {
    pEventReq event = NULL;
    int rval = -1;
    int code;
    int eno;

	//printf("reading %i from %08x(%08x--%08x) to %08x with %08x\n",maxlen,(int)filedesc,(int)filedesc->FD,(int)filedesc->Flags,(int)buffer,flags);
    	/** If closing, cant read **/
    	if (filedesc->Status == FD_S_CLOSING) return -1;

	/** If packet-mode, call packet-read routine **/
	if (flags & FD_U_PACKET) 
	    return fd_internal_ReadPkt(filedesc, buffer, maxlen, offset, flags & ~FD_U_PACKET);

	/** Check the unread-buffer first. **/
	if (filedesc->UnReadLen > 0)
	    {
	    if (maxlen > filedesc->UnReadLen) maxlen = filedesc->UnReadLen;
	    memcpy(buffer, filedesc->UnReadPtr, maxlen);
	    filedesc->UnReadLen -= maxlen;
	    filedesc->UnReadPtr += maxlen;
	    return maxlen;
	    }

    	/** If filedesc not listed as blocked, try reading now. **/
    	if (!(filedesc->Flags & FD_F_RDBLK) && !(filedesc->Flags & FD_UF_BLOCKINGIO))
    	    {
    	    if (flags & FD_U_SEEK)
		{
#ifdef HAVE_LIBZ
		if (filedesc->Flags & FD_UF_GZIP) 
		    {
		    gzseek(filedesc->GzFile, offset, SEEK_SET);
		    }
		else 
		    {
#endif
		    lseek(filedesc->FD, offset, SEEK_SET);
#ifdef HAVE_LIBZ
		    }
#endif
		}
#ifdef HAVE_LIBZ
	    if (filedesc->Flags & FD_UF_GZIP)
		{
		rval = gzread(filedesc->GzFile, buffer, maxlen);
		}
	    else 
		{
#endif
		rval = read(filedesc->FD,buffer,maxlen);
#ifdef HAVE_LIBZ
		}
#endif
	    if (rval == -1 && (errno != EWOULDBLOCK && errno != EINTR && errno != EAGAIN)) return -1;
    	    }

        /** If we need to (and may) block, create the event structure **/
      DID_BLOCK:
        if (!(flags & FD_U_NOBLOCK) && rval == -1)
            {
            event = (pEventReq)nmMalloc(sizeof(EventReq));
            if (!event) return -1;
            event->Thr = MTASK.CurrentThread;
            event->Object = (void*)filedesc;
            event->ObjType = OBJ_T_FD;
            event->EventType = EV_T_FD_READ;
            event->ReqLen = maxlen;
            event->NextPeer = event;
            event->TableIdx = evAdd(event);
	    if (event->TableIdx == -1)
	        {
	        nmFree(event, sizeof(EventReq));
	        return -1;
	        }
	    event->Status = EV_S_INPROC;
            MTASK.CurrentThread->Status = THR_S_BLOCKED;
            filedesc->Flags |= FD_F_RDBLK;
            }

        /** Call the scheduler. **/
        if (event || !(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();

        /** If event is non-null, we blocked **/
        if (event)
            {
            evRemoveIdx(event->TableIdx);
            code = event->Status;
            nmFree(event,sizeof(EventReq));
            event = NULL;
            if (code == EV_S_COMPLETE) 
                {
    	        if (flags & FD_U_SEEK)
		    {
#ifdef HAVE_LIBZ
		    if (filedesc->Flags & FD_UF_GZIP) 
			{
			gzseek(filedesc->GzFile, offset, SEEK_SET);
			}
		    else 
			{
#endif
			lseek(filedesc->FD, offset, SEEK_SET);
#ifdef HAVE_LIBZ
			}
#endif
		    }
#ifdef HAVE_LIBZ
		if (filedesc->Flags & FD_UF_GZIP)
		    {
		    rval = gzread(filedesc->GzFile, buffer, maxlen); 
		    }
		else
		    {
#endif
		    rval = read(filedesc->FD,buffer,maxlen);
#ifdef HAVE_LIBZ
		    }
#endif
		eno = errno;

    	        /** I sincerely hope this doesn't happen... **/
		if (rval == -1 && eno == EWOULDBLOCK)
    	            {
    	            puts("Got completed readability on fd but it then blocked!");
    	            goto DID_BLOCK;
    	            }
		else if (rval == -1 && (eno == EAGAIN || eno == EINTR)) goto DID_BLOCK;
                }
	    else if (code == EV_S_ERROR) rval = -1;
            }

    return rval;
    }


/*** FDUNREAD 'undoes' a read operation, with a maximum of 2048 bytes, which
 *** must all be read before another 'unread' can be performed on the same
 *** file descriptor.  Currentlly, offset and flags are ignored -- this 
 *** function should ONLY be used on descriptors that are being treated by
 *** the app as streams (no seeks performed).
 ***/
int
fdUnRead(pFile filedesc, const char* buffer, int length, int offset, int flags)
    {

    	/** Unread already done but not read yet? **/
	if (filedesc->UnReadLen > 0) return -1;

	/** Too much? **/
	if (length > 2048) return -1;

	/** Ok, copy the data. **/
	filedesc->UnReadPtr = filedesc->UnReadBuf;
	filedesc->UnReadLen = length;
	memcpy(filedesc->UnReadBuf, buffer, length);

    return length;
    }


/*** FD_INTERNAL_WRITEPKT writes a block of data as a unit, and won't quit
 *** trying until all of the data is written.  This function is triggered
 *** by a call to fdWrite with the FD_U_PACKET option set.
 ***/
int
fd_internal_WritePkt(pFile filedesc, const char* buffer, int length, int offset, int flags)
    {
    int wcnt = 0, cnt=0;

    	/** Repeatedly try until we get it all **/
	while(cnt < length)
	    {
	    wcnt = fdWrite(filedesc, buffer + cnt, length - cnt, offset, flags);
	    if (wcnt <= 0) return -1;
	    cnt += wcnt;
	    if (flags & FD_U_SEEK) offset += wcnt;
	    }

    return cnt;
    }


/*** FDWRITE writes to a file descriptor.  The flags can contain the
 *** options FD_U_NOBLOCK, FD_U_SEEK.  If FD_U_SEEK is not specified, then
 *** the seek offset is ignored.
 ***/
int
fdWrite(pFile filedesc, const char* buffer, int length, int offset, int flags)
    {
    pEventReq event = NULL;
    int rval = -1;
    int code;

    	/** If closing, cant write **/
    	if (filedesc->Status == FD_S_CLOSING) return -1;

	/** If packet-mode, call packet-read routine **/
	if (flags & FD_U_PACKET) 
	    return fd_internal_WritePkt(filedesc, buffer, length, offset, flags & ~FD_U_PACKET);

	/** Allocate the buffer? **/
	if ((filedesc->Flags & FD_UF_WRBUF) && filedesc->WrCacheBuf == NULL)
	    {
	    filedesc->WrCacheBuf = (char*)nmMalloc(MT_FD_CACHE_SIZE);
	    filedesc->WrCachePtr = filedesc->WrCacheBuf;
	    filedesc->WrCacheLen = 0;
	    }

	/** If seek set, flush the buffer **/
	if ((flags & FD_U_SEEK) && filedesc->WrCacheBuf && filedesc->WrCacheLen > 0)
	    {
	    filedesc->Flags &= ~FD_UF_WRBUF;
	    fdWrite(filedesc, filedesc->WrCacheBuf, filedesc->WrCacheLen, 0, FD_U_PACKET);
	    filedesc->WrCacheLen = 0;
	    filedesc->WrCachePtr = filedesc->WrCacheBuf;
	    filedesc->Flags |= FD_UF_WRBUF;
	    }

	/** Cache mode writes? **/
	if ((filedesc->Flags & FD_UF_WRBUF) && !(flags & FD_U_SEEK))
	    {
	    /** Will it fit in the buffer? **/
	    if (MT_FD_CACHE_SIZE - filedesc->WrCacheLen >= length)
	        {
		memcpy(filedesc->WrCachePtr, buffer, length);
		filedesc->WrCacheLen += length;
		filedesc->WrCachePtr += length;
		return length;
		}

	    /** Ok, no fit.  Write the buffer first. **/
	    if (filedesc->WrCacheLen > 0)
	        {
		filedesc->Flags &= ~FD_UF_WRBUF;
		fdWrite(filedesc, filedesc->WrCacheBuf, filedesc->WrCacheLen, 0, FD_U_PACKET);
		filedesc->WrCacheLen = 0;
		filedesc->WrCachePtr = filedesc->WrCacheBuf;
		filedesc->Flags |= FD_UF_WRBUF;
		}

	    /** Now will it fit? **/
	    if (MT_FD_CACHE_SIZE - filedesc->WrCacheLen >= length)
	        {
		memcpy(filedesc->WrCachePtr, buffer, length);
		filedesc->WrCacheLen += length;
		filedesc->WrCachePtr += length;
		return length;
		}

	    /** Ok, won't ever fit.  Write it as one big block **/
	    filedesc->Flags &= ~FD_UF_WRBUF;
	    fdWrite(filedesc, buffer, length, 0, FD_U_PACKET);
	    filedesc->Flags |= FD_UF_WRBUF;
	    return length;
	    }

    	/** If filedesc not listed as blocked, try writing now. **/
    	if (!(filedesc->Flags & FD_F_RDBLK) && !(filedesc->Flags & FD_UF_BLOCKINGIO))
    	    {
    	    if (flags & FD_U_SEEK)
		{
#ifdef HAVE_LIBZ
		if (filedesc->Flags & FD_UF_GZIP) 
		    {
		    gzseek(filedesc->GzFile, offset, SEEK_SET);
		    }
		else 
		    {
#endif
		    lseek(filedesc->FD, offset, SEEK_SET);
#ifdef HAVE_LIBZ
		    }
#endif
		}
#ifdef HAVE_LIBZ
	    if (filedesc->Flags & FD_UF_GZIP)
		{
		rval = gzwrite(filedesc->GzFile, (void*)buffer, length); 
		}
	    else
		{
#endif
		rval = write(filedesc->FD,buffer,length);
#ifdef HAVE_LIBZ
		}
#endif
    	    if (rval == -1 && (errno != EWOULDBLOCK && errno != EINTR && errno != EAGAIN)) return -1;
    	    }

        /** If we need to (and may) block, create the event structure **/
      DID_BLOCK:
        if (!(flags & FD_U_NOBLOCK) && rval == -1)
            {
            event = (pEventReq)nmMalloc(sizeof(EventReq));
            if (!event) return -1;
            event->Thr = MTASK.CurrentThread;
            event->Object = (void*)filedesc;
            event->ObjType = OBJ_T_FD;
            event->EventType = EV_T_FD_WRITE;
            event->ReqLen = length;
            event->NextPeer = event;
            event->TableIdx = evAdd(event);
	    if (event->TableIdx == -1)
	        {
	        nmFree(event, sizeof(EventReq));
	        return -1;
	        }
	    event->Status = EV_S_INPROC;
            MTASK.CurrentThread->Status = THR_S_BLOCKED;
            filedesc->Flags |= FD_F_WRBLK;
            }

        /** Call the scheduler. **/
        if (event || !(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();

        /** If event is non-null, we blocked **/
        if (event)
            {
            evRemoveIdx(event->TableIdx);
            code = event->Status;
            nmFree(event,sizeof(EventReq));
            event = NULL;
            if (code == EV_S_COMPLETE) 
                {
    	        if (flags & FD_U_SEEK)
		    {
#ifdef HAVE_LIBZ
		    if (filedesc->Flags & FD_UF_GZIP)
			{
			gzseek(filedesc->GzFile, offset, SEEK_SET);
			}
		    else 
			{
#endif
			lseek(filedesc->FD, offset, SEEK_SET);
#ifdef HAVE_LIBZ
			}
#endif
		    }
#ifdef HAVE_LIBZ
		if (filedesc->Flags & FD_UF_GZIP)
		    {
    		    rval = gzwrite(filedesc->GzFile, (void*)buffer, length); 
		    }
		else
		    {
#endif
		    rval = write(filedesc->FD,buffer,length);
#ifdef HAVE_LIBZ
		    }
#endif

    	        /** I sincerely hope this doesn't happen... **/
		if (rval == -1 && (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN))
    	            {
    	            puts("Got completed writeability on fd but it then blocked!");
    	            goto DID_BLOCK;
    	            }
                }
	    else if (code == EV_S_ERROR) rval = -1;
            }

    return rval;
    }


/*** FDQPRINTF_GROW - handles buffer undersize conditions by dumping 
 *** already-printed data to the output descriptor and going from
 *** there.
 ***/
int
fdQPrintf_Grow(char** str, size_t* size, void* arg, int req_size)
    {
    pFile filedesc = (pFile)arg;
    int incr;

	if (req_size <= *size) return 1;
	if (req_size > filedesc->PrintfBufSize) return 0;

	incr = *size - (filedesc->PrintfBuf - *str);
	fdWrite(filedesc, filedesc->PrintfBuf, incr, 0, FD_U_PACKET);
	(*size) += incr;
	(*str) -= incr;

    return 1;
    }


/*** FDQPRINTF_VA writes formatted output data, given an explicit va_list
 *** rather than doing it via a variable argument list
 ***/
int
fdQPrintf_va(pFile filedesc, const char* fmt, va_list va)
    {
    int rval;
    char* buf;
    int size;

	/** Alloc a printf buf? **/
	if (!filedesc->PrintfBuf)
	    {
	    filedesc->PrintfBufSize = FD_PRINTF_BUFSIZ;
	    filedesc->PrintfBuf = (char*)nmSysMalloc(filedesc->PrintfBufSize);
	    if (!filedesc->PrintfBuf)
		return -ENOMEM;
	    }
	buf = filedesc->PrintfBuf;
	size = filedesc->PrintfBufSize;

	/** Print it **/
	rval = qpfPrintf_va_internal(NULL, &buf, &size, fdQPrintf_Grow, filedesc, fmt, va);
	fdWrite(filedesc, filedesc->PrintfBuf, size - (filedesc->PrintfBuf - buf), 0, FD_U_PACKET);
   
    return rval;
    }


/*** FDQPRINTF writes formatted output data to the descriptor, using the
 *** qpfPrintf() quoting-printf semantics
 ***/
int
fdQPrintf(pFile filedesc, const char* fmt, ...)
    {
    va_list va;
    int rval;

	va_start(va, fmt);
	rval = fdQPrintf_va(filedesc, fmt, va);
	va_end(va);

    return rval;
    }


/*** FDPRINTF writes formatted output data to a descriptor.  It writes
 *** all data, as if FD_U_PACKET had been specified.
 ***
 *** Returns length of written data on success (>= 0), or negative on error.
 ***/
int
fdPrintf(pFile filedesc, const char* fmt, ...)
    {
    va_list va;
    int rval;

	/** Alloc a printf buf? **/
	if (!filedesc->PrintfBuf)
	    {
	    filedesc->PrintfBufSize = FD_PRINTF_BUFSIZ;
	    filedesc->PrintfBuf = (char*)nmSysMalloc(filedesc->PrintfBufSize);
	    if (!filedesc->PrintfBuf)
		return -ENOMEM;
	    }

	/** Print it. **/
	va_start(va,fmt);
	rval=xsGenPrintf_va(fdWrite, filedesc, &(filedesc->PrintfBuf), &(filedesc->PrintfBufSize), fmt, va);
	va_end(va);

    return rval;
    }


/*** FDCLOSE closes an open file.  It optionally waits until all reads
 *** and writes have completed on the FD before closing it down.
 ***/
int
fdClose(pFile filedesc, int flags)
    {
    register int i,were_entries;
    pEventReq event = NULL;

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_FDCLOSE)
	    printf("fdClose called on %p(%i)\n",filedesc,filedesc->FD);
#endif
    
	/** Need to flush the WrCacheBuf? **/
	fdUnSetOptions(filedesc, FD_UF_WRBUF);

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_FDCLOSE)
	    printf("fdClose %p(%i) -- checking if already closed\n",filedesc,filedesc->FD);
#endif

    	/** Already closing? **/
    	if (filedesc->Status == FD_S_CLOSING) return -1;
    	filedesc->Status = FD_S_CLOSING;
	
#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_FDCLOSE)
	    printf("fdClose %p(%i) -- clearing wait table\n",filedesc,filedesc->FD);
#endif

    	/** If this is immediate, clear all wait table entries. **/
    	for(were_entries=i=0;i<MTASK.nEvents;i++)
    	    {
    	    if (MTASK.EventWaitTable[i]->Object == (void*)filedesc)
    	        {
		were_entries = 1;
    	        if (flags & FD_U_IMMEDIATE) 
		    {
		    MTASK.EventWaitTable[i]->Status = EV_S_ERROR;
		    MTASK.EventWaitTable[i]->Thr->Status = THR_S_RUNNABLE;
		    }
		else
		    {
#ifdef MTASK_DEBUG
		    if(MTASK.DebugLevel & MTASK_DEBUG_FDCLOSE)
			printf("\t\t(Object,ObjType,EventType,Status)\nfdClose %p(%i) -- WT entry: %p,%i,%i,%i\n",filedesc,filedesc->FD,MTASK.EventWaitTable[i]->Object,MTASK.EventWaitTable[i]->ObjType,MTASK.EventWaitTable[i]->EventType,MTASK.EventWaitTable[i]->Status);
#endif
		    }
    	        }
    	    }

	/** If not immediate and there ARE wait table entries, wait on them **/
	if (!(flags & FD_U_IMMEDIATE) && were_entries)
	    {
#ifdef MTASK_DEBUG
	    if(MTASK.DebugLevel & MTASK_DEBUG_FDCLOSE)
		printf("fdClose %p(%i) -- there are wait table entries, waiting\n",filedesc,filedesc->FD);
#endif

	    event = (pEventReq)nmMalloc(sizeof(EventReq));
	    event->Thr = MTASK.CurrentThread;
	    event->Object = (void*)filedesc;
	    event->ObjType = OBJ_T_FD;
	    event->EventType = EV_T_FINISH;
	    event->NextPeer = event;
	    event->TableIdx = evAdd(event);
	    if (event->TableIdx == -1)
	        {
	        nmFree(event, sizeof(EventReq));
	        return -1;
	        }
	    event->Status = EV_S_INPROC;
	    MTASK.CurrentThread->Status = THR_S_BLOCKED;
	    }

	/** Call scheduler, if necessary **/
	if (event || !(MTASK.MTFlags & MT_F_NOYIELD)) 
	    {
#ifdef MTASK_DEBUG
	    if(MTASK.DebugLevel & MTASK_DEBUG_FDCLOSE)
		printf("fdClose %p(%i) -- calling scheduler\n",filedesc,filedesc->FD);
#endif
	    mtSched();
	    }

	/** Remove event... **/
	if (event)
	    {
	    evRemove(event);
	    nmFree(event,sizeof(EventReq));
	    event = NULL;
	    }

        /** Close the FD **/
	if (!(flags & FD_XU_NODST))
	    {
#ifdef HAVE_LIBZ
	    if (filedesc->Flags & FD_UF_GZIP)
		{
		gzclose(filedesc->GzFile);
		}
#endif
	    close(filedesc->FD);
	    if (filedesc->WrCacheBuf) nmFree(filedesc->WrCacheBuf, MT_FD_CACHE_SIZE);
	    if (filedesc->RdCacheBuf) nmFree(filedesc->RdCacheBuf, MT_FD_CACHE_SIZE);
	    if (filedesc->PrintfBuf) nmSysFree(filedesc->PrintfBuf);
            nmFree(filedesc,sizeof(File));
	    }
#ifdef HAVE_LIBZ
	else
	    {
	    if (filedesc->Flags & FD_UF_GZIP)
		{
		gzclose(filedesc->GzFile);
		}
	    }
#endif

    return 0;
    }


/*** FDFD returns the operating system-level file descriptor for the
 *** filedesc passed.
 ***/
int
fdFD(pFile filedesc)
    {
    return filedesc->FD;
    }


/*** FDACCESS checks to see if the thread has permission to do the
 *** given operation (W_OK, R_OK, etc.) on the file, without actually
 *** performing the operation.  Like access() but works with MTask and
 *** is based on the effective UID of the thread.
 ***/
int
fdAccess(const char* pathname, int check_ok)
    {
    int rval;

	setregid(getegid(), getgid());
	setreuid(geteuid(), getuid());
	rval = access(pathname, check_ok);
	setreuid(geteuid(), getuid());
	setregid(getegid(), getgid());

    return rval;
    }


/*** NETLISTENTCP creates a listening TCP server socket for a given
 *** service/port and with a given connection queue length (for 
 *** backlogged connections that can't be immediately serviced).
 ***/
pFile
netListenTCP(const char* service_name, int queue_length, int flags)
    {
    pFile new_fd = NULL;
    int s,arg;
    unsigned short port;
    struct servent *srv;
    struct sockaddr_in localaddr;

    	/** Create the socket. **/
	s = socket(AF_INET, SOCK_STREAM, 0);

	/** Set non-blocking **/
	arg=1;
	ioctl(s,FIONBIO,&arg);

	/** Get the port number **/
	port = htons(strtol(service_name, NULL, 10));
	if (port == 0)
	    {
	    srv = getservbyname(service_name, "tcp");
	    if (!srv)
	        {
		close(s);
		return NULL;
		}
	    port = srv->s_port;
#ifdef HAVE_ENDSERVENT
	    endservent();
#endif
	    }

	/** Setup to bind to that address. **/
	arg=1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(int));
	memset(&localaddr,0,sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = port;
	if (bind(s,(struct sockaddr*)&localaddr, sizeof(struct sockaddr_in)) FAIL)
	    {
	    close(s);
	    return NULL;
	    }

	/** Listen on the socket. **/
	if(listen(s,queue_length)==-1)
	    {
	    close(s);
	    return NULL;
	    }

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_CONNECTION_OPEN)
	    printf("Opening FD %i to accept TCP connections\n",s);
#endif
	
	/** Create the file descriptor structure **/
	new_fd = (pFile)nmMalloc(sizeof(File));
	new_fd->FD = s;
	new_fd->Status = FD_S_OPEN;
	new_fd->Flags = FD_F_RD;
	new_fd->EventCkFn = evFile;
	new_fd->ErrCode = 0;
	new_fd->UnReadLen = 0;
	new_fd->WrCacheBuf = NULL;
	new_fd->RdCacheBuf = NULL;
#ifdef HAVE_LIBZ
	new_fd->GzFile = NULL;
#endif
	new_fd->PrintfBuf = NULL;
	memcpy(&(new_fd->LocalAddr),&localaddr,sizeof(struct sockaddr_in));

    return new_fd;
    }




/*** NETACCEPTTCP accepts an incoming TCP connection from a TCP server
 *** socket, and optionally blocks if such a connection is not yet
 *** available on the listening socket.  The flag NET_U_NOBLOCK 
 *** controls such blocking action.  Add the flag NET_U_KEEPALIVE if
 *** keepalives are to be used on the connection.
 ***/
pFile
netAcceptTCP(pFile net_filedesc, int flags)
    {
    pFile connected_fd;
    int s;
    pEventReq event = NULL;
    struct sockaddr_in remoteaddr;
    int addrlen;
    int v;
    int arg;

    	/** Check to see if a connection is pending **/
	addrlen = sizeof(struct sockaddr_in);
	s = accept(net_filedesc->FD,(struct sockaddr*)&remoteaddr,&addrlen);
	if (s FAIL)
	    {
	    if (flags & NET_U_NOBLOCK) return NULL;
	    event = (pEventReq)nmMalloc(sizeof(EventReq));
	    if (!event) return NULL;
	    event->Thr = MTASK.CurrentThread;
	    event->Object = (void*)net_filedesc;
	    event->ObjType = OBJ_T_FD;
	    event->EventType = EV_T_FD_READ;
	    event->NextPeer = event;
	    event->TableIdx = evAdd(event);
	    if (event->TableIdx == -1)
	        {
	        nmFree(event, sizeof(EventReq));
	        return NULL;
	        }
	    event->Status = EV_S_INPROC;
	    MTASK.CurrentThread->Status = THR_S_BLOCKED;
	    }

	/** Call scheduler if necessary **/
	if (event || !(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();

	/** Do the accept if it failed before. **/
	if (event)
	    {
	    evRemoveIdx(event->TableIdx);
	    nmFree(event,sizeof(EventReq));
	    event = NULL;
	  RETRY_ACCEPT:
	    addrlen = sizeof(struct sockaddr_in);
	    s = accept(net_filedesc->FD,(struct sockaddr*)&remoteaddr,&addrlen);
	    if (s FAIL) 
	        {
		if (errno == EAGAIN)
		    {
		    thYield();
		    goto RETRY_ACCEPT;
		    }
		perror("accept");
		return NULL;
		}
	    }

	/** Create the new file descriptor structure **/
	connected_fd = (pFile)nmMalloc(sizeof(File));
	if (!connected_fd)
	    {
	    close(s);
	    return NULL;
	    }

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_CONNECTION_OPEN)
	    printf("Accepting connection from %i with FD %i\n",net_filedesc->FD,s);
#endif

	connected_fd->FD = s;
	connected_fd->Status = FD_S_OPEN;
	connected_fd->Flags = FD_F_RD | FD_F_WR;
	connected_fd->EventCkFn = evFile;
	connected_fd->ErrCode = 0;
	connected_fd->UnReadLen = 0;
	connected_fd->WrCacheBuf = NULL;
	connected_fd->RdCacheBuf = NULL;
#ifdef HAVE_LIBZ
	connected_fd->GzFile = NULL;
#endif
	connected_fd->PrintfBuf = NULL;
	memcpy(&(connected_fd->RemoteAddr),&remoteaddr,sizeof(struct sockaddr_in));
	memcpy(&(connected_fd->LocalAddr),&(net_filedesc->LocalAddr),sizeof(struct sockaddr_in));
	if (flags & NET_U_KEEPALIVE)
	    {
	    v = 1;
	    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v));
	    }

	arg=1;
	ioctl(connected_fd->FD,FIONBIO,&arg);

    return connected_fd;
    }


/*** NETGETREMOTEIP gets the remote IP address of a connection, useful
 *** after a netAcceptTCP().  If flags specifies NET_U_NOBLOCK, then the
 *** address is returned as a dotted quad instead of an attempt to get
 *** the associated DNS name.
 ***
 *** Currently, no DNS lookups are performed.
 ***/
char*
netGetRemoteIP(pFile net_filedesc, int flags)
    {
    if(net_filedesc->Flags & FD_F_UDP && !(net_filedesc->Flags & FD_F_CONNECTED))
	return (char*)NULL;
    return (char*)inet_ntoa(net_filedesc->RemoteAddr.sin_addr);
    }


/*** NETGETREMOTEPORT returns the port number of the remote end of
 *** the connection.  Doesn't convert it to a name from /etc/services.
 ***/
unsigned short
netGetRemotePort(pFile net_filedesc)
    {
    if(net_filedesc->Flags & FD_F_UDP && !(net_filedesc->Flags & FD_F_CONNECTED))
	return 0;
    return ntohs(net_filedesc->RemoteAddr.sin_port);
    }


/*** NETCONNECTTCP creats a client socket and connects it to a
 *** server on a given TCP service/port and host name.  The flag
 *** NET_U_NOBLOCK causes the request to return immediately even
 *** if the connection is still trying to establish.  Further
 *** reads and writes will block until the connection either
 *** establishes or fails.
 ***/
pFile
netConnectTCP(const char* host_name, const char* service_name, int flags)
    {
    pFile connected_fd;
    struct sockaddr_in remoteaddr;
    int s,arg,len;
    struct servent *srv;
    struct hostent *h;
    unsigned short port;
    unsigned int addr;
    pEventReq event = NULL;
    int code;

    	/** Create the socket **/
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s FAIL) return NULL;
	arg=1;
	ioctl(s,FIONBIO,&arg);

	/** Lookup the service name. **/
	port = htons(strtol(service_name,NULL,10));
	if (!port)
	    {
	    srv = getservbyname(service_name,"tcp");
#ifdef HAVE_ENDSERVENT
	    endservent();
#endif
	    if (srv == NULL) 
	        {
		close(s);
		return NULL;
		}
	    port = srv->s_port;
	    }

	/** Lookup the host name. **/
	addr = inet_addr(host_name);
	if (addr == 0xFFFFFFFF)
	    {
	    h = gethostbyname(host_name);
	    if (h == NULL)
	        {
		close(s);
		return NULL;
		}
	    memcpy(&addr,h->h_addr,sizeof(unsigned int));
	    }

	/** Create the structure **/
	connected_fd = (pFile)nmMalloc(sizeof(File));
	if (!connected_fd) 
	    {
	    close(s);
	    return NULL;
	    }

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_CONNECTION_OPEN)
	    printf("TCP Connection opened, using FD %i\n",s);
#endif

	connected_fd->FD = s;
	connected_fd->Status = FD_S_OPENING;
	connected_fd->Flags = FD_F_RD | FD_F_WR;
	connected_fd->EventCkFn = evFile;
	connected_fd->ErrCode = 0;
	connected_fd->UnReadLen = 0;
	connected_fd->WrCacheBuf = NULL;
	connected_fd->RdCacheBuf = NULL;
#ifdef HAVE_LIBZ
	connected_fd->GzFile = NULL;
#endif
	connected_fd->PrintfBuf = NULL;

	/** Try to connect **/
	memset(&remoteaddr,0,sizeof(struct sockaddr_in));
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_port = port;
	memcpy(&(remoteaddr.sin_addr), &addr, sizeof(unsigned int));
	memcpy(&(connected_fd->RemoteAddr),&remoteaddr,sizeof(struct sockaddr_in));
	if (connect(s,(struct sockaddr*)&remoteaddr,sizeof(struct sockaddr_in)) FAIL)
	    {
	    if (errno != EINPROGRESS)
	        {
		close(s);
		return NULL;
		}

	    /** If we can't block, return with errcode = in progress **/
	    if (flags & NET_U_NOBLOCK)
	        {
		connected_fd->ErrCode = EINPROGRESS;
		}
	    else
	        {
	        event = (pEventReq)nmMalloc(sizeof(EventReq));
	        if (!event) return NULL;
	        event->Thr = MTASK.CurrentThread;
	        event->Object = (void*)connected_fd;
	        event->ObjType = OBJ_T_FD;
	        event->EventType = EV_T_FD_WRITE;
	        event->NextPeer = event;
	        event->TableIdx = evAdd(event);
	        if (event->TableIdx == -1)
	            {
	            nmFree(event, sizeof(EventReq));
	            return NULL;
	            }
		event->Status = EV_S_INPROC;
	        MTASK.CurrentThread->Status = THR_S_BLOCKED;
		}
	    }

	/** Call the scheduler if necessary **/
	if (event || !(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();

	/** If event completed, check it. **/
	if (event)
	    {
	    code = event->Status;
	    evRemoveIdx(event->TableIdx);
	    nmFree(event,sizeof(EventReq));
	    event = NULL;
	    if (code == EV_S_ERROR)
	        {
		close(s);
		nmFree(connected_fd,sizeof(File));
		return NULL;
		}
	    len = sizeof(int);
	    arg = -1;
	    getsockopt(s, SOL_SOCKET, SO_ERROR, &arg, &len);
	    if (arg != 0)
	        {
		close(s);
		nmFree(connected_fd,sizeof(File));
		return NULL;
		}
	    }

	/** Enable keepalive? **/
	if (flags & NET_U_KEEPALIVE)
	    {
	    arg = 1;
	    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &arg, sizeof(arg));
	    }

    return connected_fd;
    }


/*** NETCLOSETCP closes a given TCP listening, server, or client
 *** socket, with the option of having the close wait until all
 *** pending i/o's on the socket have completed.
 ***/
int
netCloseTCP(pFile net_filedesc, int linger_msec, int flags)
    {
    int t,t2,rval,arg;
    struct linger l;

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_CONNECTION_CLOSE)
	    printf("Closing FD %i\n",net_filedesc->FD);
#endif

    	/** Close the file descriptor normally first. **/
	t = mtTicks();
	fdClose(net_filedesc, (flags & (FD_U_IMMEDIATE)) | FD_XU_NODST);

	/** Now we set linger on the fd. **/
	l.l_onoff = 1;
	l.l_linger = (linger_msec) / 1000;
	setsockopt(net_filedesc->FD, SOL_SOCKET, SO_LINGER, &l, sizeof(struct linger));

	/** Shutdown read and write sides of the connection. **/
	shutdown(net_filedesc->FD, 2);

	/** Try to close the socket, keeping track of timeout **/
	rval = -1;
	while(linger_msec > 0)
	    {
	    t2 = mtTicks();
	    linger_msec -= (t2-t)*(1000/MTASK.TicksPerSec);
	    t = t2;
	    rval=close(net_filedesc->FD);
	    if (rval FAIL && errno == EWOULDBLOCK)
	        {
		thSleep(1000/MTASK.TicksPerSec);
		}
	    else
	        {
		break;
		}
	    }

	/** Did the close succeed?  If not, set for forced close. **/
	if (rval FAIL)
	    {
	    l.l_onoff=0;
	    l.l_linger=0;
	    setsockopt(net_filedesc->FD, SOL_SOCKET, SO_LINGER, &l, sizeof(struct linger));
	    arg=0;
	    ioctl(net_filedesc->FD, FIONBIO, &arg);
	    close(net_filedesc->FD);
	    }

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_CONNECTION_CLOSE)
	    printf("Closed FD %i\n",net_filedesc->FD);
#endif

	/** Now we destroy the structure. **/
	if (net_filedesc->WrCacheBuf) nmFree(net_filedesc->WrCacheBuf, MT_FD_CACHE_SIZE);
	if (net_filedesc->RdCacheBuf) nmFree(net_filedesc->RdCacheBuf, MT_FD_CACHE_SIZE);
	if (net_filedesc->PrintfBuf) nmSysFree(net_filedesc->PrintfBuf);
	nmFree(net_filedesc,sizeof(File));

    return 0;
    }

/*** NETLISTENUDP creates a listening UDP server socket for a given
 *** service/port 
 ***/
pFile
netListenUDP(const char* service_name, int flags)
    {
    pFile new_fd = NULL;
    int s,arg;
    unsigned short port;
    struct servent *srv;
    struct sockaddr_in localaddr;

    	/** Create the socket. **/
	s = socket(AF_INET, SOCK_DGRAM, 0);

	/** Set non-blocking **/
	arg=1;
	ioctl(s,FIONBIO,&arg);

	/** Get the port number **/
	port = htons(strtol(service_name, NULL, 10));
	if (port == 0)
	    {
	    srv = getservbyname(service_name, "udp");
	    if (!srv)
	        {
		close(s);
		return NULL;
		}
	    port = srv->s_port;
#ifdef HAVE_ENDSERVENT
	    endservent();
#endif
	    }

	/** Setup to bind to that address. **/
	arg=1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(int));
	memset(&localaddr,0,sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = port;
	if (bind(s,(struct sockaddr*)&localaddr, sizeof(struct sockaddr_in)) FAIL)
	    {
	    close(s);
	    return NULL;
	    }

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_CONNECTION_OPEN)
	    printf("Opening FD %i to accept UDP connections\n",s);
#endif
	
	/** Create the file descriptor structure **/
	new_fd = (pFile)nmMalloc(sizeof(File));
	new_fd->FD = s;
	new_fd->Status = FD_S_OPEN;
	new_fd->Flags = FD_F_RD | FD_F_WR | FD_F_UDP;
	new_fd->EventCkFn = evFile;
	new_fd->ErrCode = 0;
	new_fd->UnReadLen = 0;
	new_fd->WrCacheBuf = NULL;
	new_fd->RdCacheBuf = NULL;
#ifdef HAVE_LIBZ
	new_fd->GzFile = NULL;
#endif
	new_fd->PrintfBuf = NULL;
	memcpy(&(new_fd->LocalAddr),&localaddr,sizeof(struct sockaddr_in));

    return new_fd;
    }

/*** NETCONNECTUDP creats a client socket and connects it to a
 *** server on a given UDP service/port and host name.  The flag
 *** NET_U_NOBLOCK causes the request to return immediately even
 *** if the connection is still trying to establish.  Further
 *** reads and writes will block until the connection either
 *** establishes or fails.
 ***/
pFile
netConnectUDP(const char* host_name, const char* service_name, int flags)
    {
    pFile connected_fd;
    struct sockaddr_in remoteaddr;
    int s,arg,len;
    struct servent *srv;
    struct hostent *h;
    unsigned short port;
    unsigned int addr;
    pEventReq event = NULL;
    int code;

    	/** Create the socket **/
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s FAIL) return NULL;
	arg=1;
	ioctl(s,FIONBIO,&arg);

	/** Lookup the service name. **/
	port = htons(strtol(service_name,NULL,10));
	if (!port)
	    {
	    srv = getservbyname(service_name,"udp");
#ifdef HAVE_ENDSERVENT
	    endservent();
#endif
	    if (srv == NULL) 
	        {
		close(s);
		return NULL;
		}
	    port = srv->s_port;
	    }

	/** Lookup the host name. **/
	addr = inet_addr(host_name);
	if (addr == 0xFFFFFFFF)
	    {
	    h = gethostbyname(host_name);
	    if (h == NULL)
	        {
		close(s);
		return NULL;
		}
	    memcpy(&addr,h->h_addr,sizeof(unsigned int));
	    }

	/** Create the structure **/
	connected_fd = (pFile)nmMalloc(sizeof(File));
	if (!connected_fd) 
	    {
	    close(s);
	    return NULL;
	    }

#ifdef MTASK_DEBUG
	if(MTASK.DebugLevel & MTASK_DEBUG_SHOW_CONNECTION_OPEN)
	    printf("UDP Connection opened, using FD %i\n",s);
#endif

	connected_fd->FD = s;
	connected_fd->Status = FD_S_OPENING;
	connected_fd->Flags = FD_F_RD | FD_F_WR | FD_F_UDP | FD_F_CONNECTED;
	connected_fd->EventCkFn = evFile;
	connected_fd->ErrCode = 0;
	connected_fd->UnReadLen = 0;
	connected_fd->WrCacheBuf = NULL;
	connected_fd->RdCacheBuf = NULL;
#ifdef HAVE_LIBZ
	connected_fd->GzFile = NULL;
#endif
	connected_fd->PrintfBuf = NULL;

	/** Try to connect **/
	memset(&remoteaddr,0,sizeof(struct sockaddr_in));
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_port = port;
	memcpy(&(remoteaddr.sin_addr), &addr, sizeof(unsigned int));
	memcpy(&(connected_fd->RemoteAddr),&remoteaddr,sizeof(struct sockaddr_in));
	if (connect(s,(struct sockaddr*)&remoteaddr,sizeof(struct sockaddr_in)) FAIL)
	    {
	    if (errno != EINPROGRESS)
	        {
		close(s);
		return NULL;
		}

	    /** If we can't block, return with errcode = in progress **/
	    if (flags & NET_U_NOBLOCK)
	        {
		connected_fd->ErrCode = EINPROGRESS;
		}
	    else
	        {
	        event = (pEventReq)nmMalloc(sizeof(EventReq));
	        if (!event) return NULL;
	        event->Thr = MTASK.CurrentThread;
	        event->Object = (void*)connected_fd;
	        event->ObjType = OBJ_T_FD;
	        event->EventType = EV_T_FD_WRITE;
	        event->NextPeer = event;
	        event->TableIdx = evAdd(event);
	        if (event->TableIdx == -1)
	            {
	            nmFree(event, sizeof(EventReq));
	            return NULL;
	            }
		event->Status = EV_S_INPROC;
	        MTASK.CurrentThread->Status = THR_S_BLOCKED;
		}
	    }

	/** Call the scheduler if necessary **/
	if (event || !(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();

	/** If event completed, check it. **/
	if (event)
	    {
	    code = event->Status;
	    evRemoveIdx(event->TableIdx);
	    nmFree(event,sizeof(EventReq));
	    event = NULL;
	    if (code == EV_S_ERROR)
	        {
		close(s);
		nmFree(connected_fd,sizeof(File));
		return NULL;
		}
	    len = sizeof(int);
	    arg = -1;
	    getsockopt(s, SOL_SOCKET, SO_ERROR, &arg, &len);
	    if (arg != 0)
	        {
		close(s);
		nmFree(connected_fd,sizeof(File));
		return NULL;
		}
	    }

    return connected_fd;
    }

/*** NETRECVUDP recieves from a UDP file descriptor.  The flags can contain the
 *** option FD_U_NOBLOCK. 
 ***/
int
netRecvUDP(pFile filedesc, char* buffer, int maxlen, int flags, struct sockaddr_in* from, char** host, int* port)
    {
    pEventReq event = NULL;
    int rval = -1;
    int code;
    int eno;
    struct sockaddr_in remotehost;
    socklen_t fromlen=sizeof(struct sockaddr_in);
    int recvflags=0;

	/** only UDP sockets **/
	if(!(filedesc->Flags & FD_F_UDP))
	    {
	    return -1;
	    }

	remotehost.sin_addr.s_addr=0;
	remotehost.sin_port=0;
    
	if(!from)
	    {
	    from=&remotehost;
	    }

    	if (filedesc->Status == FD_S_CLOSING) return -1;

    	/** If filedesc not listed as blocked, try reading now. **/
    	if (!(filedesc->Flags & FD_F_RDBLK))
    	    {
	    rval = recvfrom(filedesc->FD,buffer,maxlen,recvflags,(struct sockaddr*)from,&fromlen);
    	    if (rval == -1 && errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR) return -1;
    	    }

        /** If we need to (and may) block, create the event structure **/
      DID_BLOCK:
        if (!(flags & FD_U_NOBLOCK) && rval == -1)
            {
            event = (pEventReq)nmMalloc(sizeof(EventReq));
            if (!event) return -1;
            event->Thr = MTASK.CurrentThread;
            event->Object = (void*)filedesc;
            event->ObjType = OBJ_T_FD;
            event->EventType = EV_T_FD_READ;
            event->ReqLen = maxlen;
            event->NextPeer = event;
            event->TableIdx = evAdd(event);
	    if (event->TableIdx == -1)
	        {
	        nmFree(event, sizeof(EventReq));
	        return -1;
	        }
	    event->Status = EV_S_INPROC;
            MTASK.CurrentThread->Status = THR_S_BLOCKED;
            filedesc->Flags |= FD_F_RDBLK;
            }

        /** Call the scheduler. **/
        if (event || !(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();

        /** If event is non-null, we blocked **/
        if (event)
            {
            evRemoveIdx(event->TableIdx);
            code = event->Status;
            nmFree(event,sizeof(EventReq));
            event = NULL;
            if (code == EV_S_COMPLETE) 
                {
		rval = recvfrom(filedesc->FD,buffer,maxlen,recvflags,(struct sockaddr*)from,&fromlen);
		eno = errno;

    	        /** I sincerely hope this doesn't happen... **/
    	        if (rval == -1 && eno == EWOULDBLOCK) 
    	            {
    	            puts("Got completed readability on fd but it then blocked!");
    	            goto DID_BLOCK;
    	            }
		else if (rval == -1 && (eno == EAGAIN || eno == EINTR)) goto DID_BLOCK;
                }
	    else if (code == EV_S_ERROR) rval = -1;
            }

	if(host)
	    {
	    *host = (char*)inet_ntoa(from->sin_addr);
	    }
	if(port)
	    {
	    *port = ntohs(from->sin_port);
	    }

    return rval;
    }

/*** NETSENDUDP recieves from a UDP file descriptor.  The flags can contain the
 *** option FD_U_NOBLOCK. 
 ***/
int
netSendUDP(pFile filedesc, char* buffer, int maxlen, int flags, struct sockaddr_in* from, char* host, int port)
    {
    pEventReq event = NULL;
    int rval = -1;
    int code;
    int eno;
    struct sockaddr_in remotehost;
    socklen_t fromlen=sizeof(struct sockaddr_in);
    int sendflags=0;

	/** only UDP sockets **/
	if(!(filedesc->Flags & FD_F_UDP))
	    {
	    printf("not UDP\n");
	    return -1;
	    }

	if(filedesc->Flags & FD_F_CONNECTED)
	    {
	    from = NULL;
	    fromlen = 0;
	    }
	else
	    {
	    if(!from)
		{
		from=&remotehost;
		from->sin_family=AF_INET;
		if(!(host && port))
		    {
		    /** either from or host and port are required **/
		    printf("host and port required\n");
		    return -1;
		    }
		if(inet_aton(host,&(from->sin_addr))==0)
		    {
		    printf("invalid host\n");
		    return -1;
		    }
		from->sin_port = htons(port);
		}
	    }

    	if (filedesc->Status == FD_S_CLOSING) return -1;

    	/** If filedesc not listed as blocked, try reading now. **/
    	if (!(filedesc->Flags & FD_F_WRBLK))
    	    {
	    rval = sendto(filedesc->FD,buffer,maxlen,sendflags,(struct sockaddr*)from,fromlen);
    	    if (rval == -1 && errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR) return -1;
    	    }

        /** If we need to (and may) block, create the event structure **/
      DID_BLOCK:
        if (!(flags & FD_U_NOBLOCK) && rval == -1)
            {
            event = (pEventReq)nmMalloc(sizeof(EventReq));
            if (!event) return -1;
            event->Thr = MTASK.CurrentThread;
            event->Object = (void*)filedesc;
            event->ObjType = OBJ_T_FD;
            event->EventType = EV_T_FD_WRITE;
            event->ReqLen = maxlen;
            event->NextPeer = event;
            event->TableIdx = evAdd(event);
	    if (event->TableIdx == -1)
	        {
	        nmFree(event, sizeof(EventReq));
	        return -1;
	        }
	    event->Status = EV_S_INPROC;
            MTASK.CurrentThread->Status = THR_S_BLOCKED;
            filedesc->Flags |= FD_F_WRBLK;
            }

        /** Call the scheduler. **/
        if (event || !(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();

        /** If event is non-null, we blocked **/
        if (event)
            {
            evRemoveIdx(event->TableIdx);
            code = event->Status;
            nmFree(event,sizeof(EventReq));
            event = NULL;
            if (code == EV_S_COMPLETE) 
                {
		rval = sendto(filedesc->FD,buffer,maxlen,sendflags,(struct sockaddr*)from,fromlen);
		eno = errno;

    	        /** I sincerely hope this doesn't happen... **/
    	        if (rval == -1 && eno == EWOULDBLOCK) 
    	            {
    	            puts("Got completed readability on fd but it then blocked!");
    	            goto DID_BLOCK;
    	            }
		else if (rval == -1 && (eno == EAGAIN || eno == EINTR)) goto DID_BLOCK;
                }
	    else if (code == EV_S_ERROR) rval = -1;
            }

    return rval;
    }



/*** SYCREATESEM creates a new semaphore, initialized with a given initial count
 *** and optional flags.
 ***/
pSemaphore
syCreateSem(int init_count, int flags)
    {
    pSemaphore this;

    	/** Allocate the semaphore **/
	this = (pSemaphore)nmMalloc(sizeof(Semaphore));
	if (!this) return NULL;

	/** Fill in its entries. **/
	this->EventCkFn = evSemaphore;
	this->Count = init_count;
	this->Flags = flags;

    return this;
    }


/*** SYDESTROYSEM destroys an existing semaphore, optionally forcing all waiting
 *** events to exit on the thing with error.  Otherwise, the close operation
 *** will be deferred until all waiting processes are satisfied, although no new
 *** processes may wait on the semaphore.
 ***/
int
syDestroySem(pSemaphore sem, int flags)
    {
    register int i,cnt;

    	/** Immediate?  If so, error all events on this **/
	for(cnt=i=0;i<MTASK.nEvents;i++)
	    {
	    if (MTASK.EventWaitTable[i]->Object == (void*)sem)
		{
	        if (flags & SEM_U_HARDCLOSE)
	            {
		    MTASK.EventWaitTable[i]->Status = EV_S_ERROR;
		    MTASK.EventWaitTable[i]->Thr->Status = THR_S_RUNNABLE;
		    }
		cnt++;
		}
	    }

	/** Can we go ahead and discard the structure? **/
        if ((flags & SEM_U_HARDCLOSE) || cnt==0)
	    {
	    nmFree(sem,sizeof(Semaphore));
	    }
	else
	    {
	    sem->Flags |= SEM_F_CLOSING;
	    }

    return 0;
    }


/*** SYPOSTSEM posts one or more count(s) to the semaphore, possibly
 *** completing one or more events on the thing.
 ***/
int
syPostSem(pSemaphore sem, int cnt, int flags)
    {
    int totalcnt;
    register int i,activate_cnt;

    	/** So how many total counts do we have to give away? **/
	totalcnt = cnt + sem->Count;

	/** See if we can free up any waiting threads. **/
	for(activate_cnt=i=0;i<MTASK.nEvents && totalcnt>0;i++)
	    {
	    if (MTASK.EventWaitTable[i]->Object == (void*)sem)
	        {
		if (MTASK.EventWaitTable[i]->ReqLen <= totalcnt)
		    {
		    MTASK.EventWaitTable[i]->Status = EV_S_COMPLETE;
		    MTASK.EventWaitTable[i]->Thr->Status = THR_S_RUNNABLE;
		    totalcnt -= MTASK.EventWaitTable[i]->ReqLen;
		    activate_cnt++;
		    }
		}
	    }

	/** Ok, set the semaphore count to what remains. **/
	sem->Count = totalcnt;

	/** IF we freed up processes or noyield isn't turned on, call mtsched **/
	if (activate_cnt > 0 || !(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();

    return 0;
    }


/*** SYGETSEM retrieves one or more counts from the semaphore, possibly
 *** blocking to wait for the availability of counts.  If the SEM_U_NOBLOCK
 *** flag is set, this routine will not wait but will error out if insufficient
 *** counts are available.
 ***/
int
syGetSem(pSemaphore sem, int cnt, int flags)
    {
    pEventReq event;
    int code;

    	/** Is semaphore available? **/
	if (sem->Count >= cnt)
	    {
	    sem->Count -= cnt;
	    if (!(MTASK.MTFlags & MT_F_NOYIELD)) mtSched();
	    return 0;
	    }

	/** Not available but can't block? **/
	if (flags & SEM_U_NOBLOCK)
	    {
	    return -1;
	    }

	/** Ok, not available but we should wait on the thing **/
	event = (pEventReq)nmMalloc(sizeof(EventReq));
	event->Object = (void*)sem;
	event->ObjType = OBJ_T_SEM;
	event->EventType = EV_T_SEM_GET;
	event->ReqLen = cnt;
	event->Thr = MTASK.CurrentThread;
	event->Thr->Status = THR_S_BLOCKED;
	event->Status = EV_S_INPROC;
	event->NextPeer = event;
	event->TableIdx = evAdd(event);
        if (event->TableIdx == -1)
            {
            nmFree(event, sizeof(EventReq));
            return -1;
            }
	mtSched();

	/** Ok, got completion.  Check error or ok. **/
	if (event->Status == EV_S_ERROR) code = -1;
	else code = 0;
	evRemoveIdx(event->TableIdx);
	nmFree(event,sizeof(EventReq));

    return code;
    }

