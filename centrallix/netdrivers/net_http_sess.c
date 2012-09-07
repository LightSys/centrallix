#include "net_http.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	net_http.h, net_http.c, net_http_conn.c, net_http_sess.c, net_http_osml.c, net_http_app.c			*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 8, 1998  					*/
/* Description:	Network handler providing an HTTP interface to the 	*/
/*		Centrallix and the ObjectSystem.			*/
/************************************************************************/

 
 
/*** nht_internal_LinkSess() - link to a session, thus increasing its link
 *** count.
 ***/
int
nht_internal_LinkSess(pNhtSessionData sess)
    {
    if (sess->LinkCnt <= 0 || sess->LinkCnt > 65535)
	mssError(1,"NHT","Bark!  Bad link count %d for session %s", sess->LinkCnt, sess->Cookie);
    sess->LinkCnt++;
    sess->LastAccess = NHT.AccCnt++;
    return 0;
    }


/*** nht_internal_UnlinkSess_r() - does the work of freeing an individual 
 *** handle within the session.
 ***/
int
nht_internal_UnlinkSess_r(void* v)
    {
    if (ISMAGIC(v, MGK_OBJSESSION)) 
	{
	objCloseSession((pObjSession)v);
	}
    return 0;
    }


/*** nht_internal_UnlinkSess() - free a session when its link count reaches 0.
 ***/
int
nht_internal_UnlinkSess(pNhtSessionData sess)
    {
    pXString errmsg;
    pNhtConnTrigger trg;
    pNhtControlMsg cm;
    int i;
    pNhtQuery nht_query;

	/** Bump the link cnt down **/
	sess->LinkCnt--;

	/** Time to close the session down? **/
	if (sess->LinkCnt <= 0)
	    {
	    printf("NHT: releasing session for username [%s], cookie [%s]\n", sess->Username, sess->Cookie);

	    /** Kill the inactivity timers first so we don't get re-entered **/
	    nht_internal_RemoveWatchdog(sess->WatchdogTimer);
	    nht_internal_RemoveWatchdog(sess->InactivityTimer);

	    /** Decrement user session count **/
	    sess->User->SessionCnt--;

	    /** Remove the session from the global session list. **/
	    xhRemove(&(NHT.CookieSessions), sess->Cookie);
	    xaRemoveItem(&(NHT.Sessions), xaFindItem(&(NHT.Sessions), (void*)sess));

	    /** First, close all open handles. **/
	    xhnClearHandles(&(sess->Hctx), nht_internal_UnlinkSess_r);

	    /** Close the master session. **/
	    objCloseSession(sess->ObjSess);

	    /** Destroy the errors semaphore **/
	    syDestroySem(sess->Errors, SEM_U_HARDCLOSE);
	    syDestroySem(sess->ControlMsgs, SEM_U_HARDCLOSE);

	    /** Clear the errors list **/
	    while(sess->ErrorList.nItems)
		{
		errmsg = (pXString)(sess->ErrorList.Items[0]);
		xaRemoveItem(&sess->ErrorList, 0);
		xsDeInit(errmsg);
		nmFree(errmsg,sizeof(XString));
		}

	    /** Clear the triggers list **/
	    while(sess->Triggers.nItems)
		{
		trg = (pNhtConnTrigger)(sess->Triggers.Items[0]);
		trg->LinkCnt--;
		if (trg->LinkCnt <= 0)
		    {
		    syDestroySem(trg->TriggerSem,SEM_U_HARDCLOSE);
		    xaRemoveItem(&(sess->Triggers), 0);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    }
		}

	    /** Clear control msg list **/
	    while(sess->ErrorList.nItems)
		{
		cm = (pNhtControlMsg)(sess->ControlMsgsList.Items[0]);
		xaRemoveItem(&sess->ControlMsgsList, 0);
		nht_internal_FreeControlMsg(cm);
		}

	    /** Query data **/
	    for(i=0;i<sess->OsmlQueryList.nItems;i++)
		{
		nht_query = (pNhtQuery)(sess->OsmlQueryList.Items[i]);
		if (nht_query->ParamData) stFreeInf_ne(nht_query->ParamData);
		if (nht_query->ParamList) expFreeParamList(nht_query->ParamList);
		nmFree(nht_query, sizeof(NhtQuery));
		}

	    /** Dealloc the xarrays and such **/
	    xaDeInit(&(sess->Triggers));
	    xaDeInit(&(sess->ErrorList));
	    xaDeInit(&(sess->ControlMsgsList));
	    xaDeInit(&(sess->OsmlQueryList));
	    xhnDeInitContext(&(sess->Hctx));
	    nmFree(sess, sizeof(NhtSessionData));
	    }

    return 0;
    }


/*** nht_internal_Watchdog() - manages the session watchdog timers.  These
 *** timers cause sessions to automatically time out if a certain operation
 *** isn't performed every so often.
 ***
 *** fyi - if the tick comparison arithmetic looks funny, it's because we
 *** need to use modulus arithmetic to prevent failure of this routine if
 *** (gasp!) Centrallix has been running for several hundred days (at
 *** CLK_TCK==100, the 32bit tick counter wraps at just under 500 days).
 ***/
void
nht_internal_Watchdog(void* v)
    {
    unsigned int cur_tick, next_tick;
    int i;
    pNhtTimer t;
    EventReq timer_event, semaphore_event;
    pEventReq ev[2] = {&timer_event, &semaphore_event};
    pNhtTimer expired_timer;

	/** Set the thread's name **/
	thSetName(NULL,"NHT Watchdog Manager Task");

	/** Enter our loop of compiling timer data and then waiting on
	 ** either an expiration or on the update semaphore (which tells us
	 ** that the timer data changed)
	 **/
	while(1)
	    {
	    /** Acquire the timer data mutex **/
	    syGetSem(NHT.TimerDataMutex, 1, 0);

	    /** Scan the timer list for next expiration. **/
	    cur_tick = mtLastTick();
	    next_tick = cur_tick + 0x7FFFF;
	    for(i=0;i<NHT.Timers.nItems;i++)
		{
		t = (pNhtTimer)(NHT.Timers.Items[i]);
		if (t->ExpireTick - next_tick > (unsigned int)0x80000000)
		    {
		    next_tick = t->ExpireTick;
		    }
		}
	    syPostSem(NHT.TimerDataMutex, 1, 0);

	    /** Has the next tick already expired?  If not, wait for it **/
	    if (cur_tick - next_tick > (unsigned int)0x80000000)
		{
		/** Setup our event list for the multiwait operation **/
		ev[0]->Object = NULL;
		ev[0]->ObjType = OBJ_T_MTASK;
		ev[0]->EventType = EV_T_MT_TIMER;
		ev[0]->ReqLen = (next_tick - cur_tick)*(1000/NHT.ClkTck);
		ev[1]->Object = NHT.TimerUpdateSem;
		ev[1]->ObjType = OBJ_T_SEM;
		ev[1]->EventType = EV_T_SEM_GET;
		ev[1]->ReqLen = 1;
		thMultiWait(2, ev);

		/** If the semaphore completed, reset the process... **/
		if (semaphore_event.Status == EV_S_COMPLETE) 
		    {
		    /** Clean out any posted updates... **/
		    while(syGetSem(NHT.TimerUpdateSem, 1, SEM_U_NOBLOCK) >= 0) ;

		    /** Restart the loop and wait for more timers **/
		    continue;
		    }
		}

	    /** Ok, one or more events need to be fired off.
	     ** We search this over again each time we look for another
	     ** timer, since the expire routine may have messed with the
	     ** timer table.
	     **/
	    cur_tick = mtLastTick();
	    do  {
		syGetSem(NHT.TimerDataMutex, 1, 0);
		expired_timer = NULL;

		/** Look for a single expired timer **/
		for(i=0;i<NHT.Timers.nItems;i++)
		    {
		    t = (pNhtTimer)(NHT.Timers.Items[i]);
		    if (t->ExpireTick - cur_tick > (unsigned int)0x80000000)
			{
			xaRemoveItem(&(NHT.Timers), i);
			expired_timer = t;
			break;
			}
		    }
		syPostSem(NHT.TimerDataMutex, 1, 0);

		/** Here is how the watchdog barks... **/
		if (expired_timer)
		    {
		    expired_timer->ExpireFn(expired_timer->ExpireArg, expired_timer->Handle);
		    xhnFreeHandle(&(NHT.TimerHctx), expired_timer->Handle);
		    nmFree(expired_timer, sizeof(NhtTimer));
		    }
		}
		while(expired_timer); /* end do loop */
	    }


    thExit();
    }


/*** nht_internal_AddWatchdog() - adds a timer to the watchdog timer list.
 ***/
handle_t
nht_internal_AddWatchdog(int timer_msec, int (*expire_fn)(), void* expire_arg)
    {
    pNhtTimer t;

	/** First, allocate ourselves a timer. **/
	t = (pNhtTimer)nmMalloc(sizeof(NhtTimer));
	if (!t) return XHN_INVALID_HANDLE;
	t->ExpireFn = expire_fn;
	t->ExpireArg = expire_arg;
	t->Time = timer_msec;
	t->ExpireTick = mtLastTick() + (timer_msec*NHT.ClkTck/1000);
        t->Handle = xhnAllocHandle(&(NHT.TimerHctx), t);

	/** Add it to the list... **/
	syGetSem(NHT.TimerDataMutex, 1, 0);
	xaAddItem(&(NHT.Timers), (void*)t);
	syPostSem(NHT.TimerDataMutex, 1, 0);

	/** ... and post to the semaphore so the watchdog thread sees it **/
	syPostSem(NHT.TimerUpdateSem, 1, 0);

    return t->Handle;
    }


/*** nht_internal_RemoveWatchdog() - removes a timer from the timer list.
 *** Returns 0 if it successfully removed it, or -1 if the timer did not
 *** exist.  In either case, the caller MUST consider the pointer 't'
 *** as invalid once the call returns.
 ***/
int
nht_internal_RemoveWatchdog(handle_t th)
    {
    int idx;
    pNhtTimer t = xhnHandlePtr(&(NHT.TimerHctx), th);

	if (!t) return -1;

	/** Remove the thing from the list, if it exists. **/
	syGetSem(NHT.TimerDataMutex, 1, 0);
	idx = xaFindItem(&(NHT.Timers), (void*)t);
	if (idx >= 0) xaRemoveItem(&(NHT.Timers), idx);
	syPostSem(NHT.TimerDataMutex, 1, 0);
	if (idx < 0) return -1;

	/** Free the timer. **/
	xhnFreeHandle(&(NHT.TimerHctx), th);
	nmFree(t, sizeof(NhtTimer));

	/** ... and let the watchdog thread know about it **/
	syPostSem(NHT.TimerUpdateSem, 1, 0);

    return 0;
    }


/*** nht_internal_ResetWatchdog() - resets the countdown timer for a 
 *** given watchdog to its original value.  Returns 0 if successful,
 *** or -1 if the thing is no longer valid.
 ***/
int
nht_internal_ResetWatchdog(handle_t th)
    {
    int idx;
    pNhtTimer t = xhnHandlePtr(&(NHT.TimerHctx), th);

	if (!t) return -1;

	/** Find the timer on the list, if it exists. **/
	syGetSem(NHT.TimerDataMutex, 1, 0);
	idx = xaFindItem(&(NHT.Timers), (void*)t);
	if (idx >= 0) t->ExpireTick = mtLastTick() + (t->Time*NHT.ClkTck/1000);
	syPostSem(NHT.TimerDataMutex, 1, 0);

	/** ... and let the watchdog thread know about it **/
	syPostSem(NHT.TimerUpdateSem, 1, 0);

    return (idx >= 0)?0:-1;
    }


/*** nht_internal_WTimeout() - this routine is called when the session 
 *** watchdog timer expires for a session.  The response is to remove the
 *** session.
 ***/
int
nht_internal_WTimeout(void* sess_v)
    {
    pNhtSessionData sess = (pNhtSessionData)sess_v;

	/** Unlink from the session. **/
	nht_internal_UnlinkSess(sess);

    return 0;
    }


/*** nht_internal_ITimeout() - this routine is called when the session 
 *** inactivity timer expires for a session.  The response is to remove the
 *** session.
 ***/
int
nht_internal_ITimeout(void* sess_v)
    {
    pNhtSessionData sess = (pNhtSessionData)sess_v;

	/** Right now, does the same as WTimeout, so call that. **/
	nht_internal_WTimeout(sess);

    return 0;
    }


/*** nht_internal_DiscardASession - search for sessions by a given user,
 *** and discard a suitably old one.
 ***/
int
nht_internal_DiscardASession(pNhtUser usr)
    {
    int i;
    pNhtSessionData search_s, old_s = NULL;

	for(i=0;i<xaCount(&NHT.Sessions);i++)
	    {
	    search_s = xaGetItem(&NHT.Sessions, i);
	    if (!old_s || old_s->LastAccess > search_s->LastAccess)
		old_s = search_s;
	    }
	if (old_s)
	    nht_internal_UnlinkSess(old_s);

    return 0;
    }
