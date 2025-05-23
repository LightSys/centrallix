#include "net_http.h"
#include "application.h"

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

 
 
/*** nht_i_CreateCookie - generate a random string value that can
 *** be used as an HTTP cookie.
 ***/
int
nht_i_CreateCookie(char* ck, int cksize, int using_tls)
    {
    int key[4];

	cxssGenerateKey((unsigned char*)key, sizeof(key));
	snprintf(ck,cksize,"%s=%8.8x%8.8x%8.8x%8.8x",
		using_tls?NHT.TlsSessionCookie:NHT.SessionCookie,
		key[0], key[1], key[2], key[3]);

    return 0;
    }


/*** nht_i_LinkSess() - link to a session, thus increasing its link
 *** count.
 ***/
int
nht_i_LinkSess(pNhtSessionData sess)
    {
    if (sess->LinkCnt <= 0 || sess->LinkCnt > 65535)
	mssError(1,"NHT","Bark!  Bad link count %d for session %s", sess->LinkCnt, sess->Cookie);
    sess->LinkCnt++;
    sess->LastAccess = NHT.AccCnt++;
    return 0;
    }


/*** nht_i_UnlinkSess_r() - does the work of freeing an individual 
 *** handle within the session.
 ***/
int
nht_i_UnlinkSess_r(void* v)
    {
    if (ISMAGIC(v, MGK_OBJSESSION)) 
	{
	objCloseSession((pObjSession)v);
	}
    return 0;
    }


/*** nht_i_AllocSession() - create a new session.
 ***/
pNhtSessionData
nht_i_AllocSession(char* usrname, int using_tls)
    {
    pNhtUser usr;
    pNhtSessionData nsess;
    int akey[4];

	/** Does the user context exist yet? **/
	usr = (pNhtUser)xhLookup(&(NHT.UsersByName), usrname);
	if (!usr)
	    {
	    usr = (pNhtUser)nmMalloc(sizeof(NhtUser));
	    usr->SessionCnt = 0;
	    xaInit(&usr->Sessions, 16);
	    objCurrentDate(&(usr->FirstActivity));
	    objCurrentDate(&(usr->LastActivity));
	    strtcpy(usr->Username, usrname, sizeof(usr->Username));
	    xhAdd(&(NHT.UsersByName), usr->Username, (void*)(usr));
	    xaAddItem(&NHT.UsersList, (void*)usr);
	    }
	usr->SessionCnt++;

	/** Too many sessions already for this user? **/
	if (usr->SessionCnt >= NHT.UserSessionLimit)
	    {
	    nht_i_DiscardASession(usr);
	    }

	/** Create the session. **/
	nsess = (pNhtSessionData)nmMalloc(sizeof(NhtSessionData));
	memset(nsess, 0, sizeof(NhtSessionData));
	strtcpy(nsess->Username, mssUserName(), sizeof(nsess->Username));
	strtcpy(nsess->Password, mssPassword(), sizeof(nsess->Password));
	thGetSecContext(NULL, &(nsess->SecurityContext));
	nsess->User = usr;
	xaAddItem(&usr->Sessions, (void*)nsess);
	nsess->Session = thGetParam(NULL,"mss");
	mssLinkSession(nsess->Session);
	nsess->IsNewCookie = 1;
	nsess->ObjSess = objOpenSession("/");
	nsess->Errors = syCreateSem(0,0);
	nsess->ControlMsgs = syCreateSem(0,0);

	/** Reference count:
	 ** - one for the caller (connection)
	 ** - one for the watchdog manager (session persistence)
	 **/
	nsess->LinkCnt = 2;
	/*xaInit(&nsess->Triggers,16);*/
	xaInit(&nsess->ErrorList,16);
	xaInit(&nsess->ControlMsgsList,16);
	xaInit(&nsess->OsmlQueryList,64);
	xaInit(&nsess->AppGroups,16);
	xhnInitContext(&(nsess->Hctx));
	nsess->CachedApps = (pXHashTable)nmMalloc(sizeof(XHashTable));
	xhInit(nsess->CachedApps, 127, 4);
	nsess->S_ID = NHT.S_ID_Count++;
	snprintf(nsess->S_ID_Text, sizeof(nsess->S_ID_Text), "%lld", nsess->S_ID);
	objCurrentDate(&(nsess->FirstActivity));
	objCurrentDate(&(nsess->LastActivity));

	/** Create the cookie and the CSRF token (akey) **/
	nht_i_CreateCookie(nsess->Cookie, sizeof(nsess->Cookie), using_tls);
	cxssGenerateKey((unsigned char*)akey, sizeof(akey));
	sprintf(nsess->SKey, "%8.8x%8.8x%8.8x%8.8x", akey[0], akey[1], akey[2], akey[3]);

	/** List this session **/
	xhAdd(&(NHT.SessionsByID), nsess->S_ID_Text, (void*)nsess);
	xhAdd(&(NHT.CookieSessions), nsess->Cookie, (void*)nsess);
	xaAddItem(&(NHT.Sessions), (void*)nsess);

	/** Watchdog and Inactivity timers.  We check the ref count after adding the first
	 ** timer to make sure session persistence still has a ref to the session.  If it
	 ** has already decremented, then only the caller has a ref, and we don't add the
	 ** second timer.
	 **/
	nsess->WatchdogTimer = nht_i_AddWatchdog(NHT.WatchdogTime*1000, nht_i_WTimeout, (void*)nsess);
	if (nsess->LinkCnt > 1)
	    nsess->InactivityTimer = nht_i_AddWatchdog(NHT.InactivityTime*1000, nht_i_ITimeout, (void*)nsess);

    return nsess;
    }


/*** nht_i_UnlinkSess() - free a session when its link count reaches 0.
 ***/
int
nht_i_UnlinkSess(pNhtSessionData sess)
    {
    pXString errmsg;
    /*pNhtConnTrigger trg;*/
    pNhtControlMsg cm;
    int i;
    pNhtQuery nht_query;

	/** Bump the link cnt down **/
	sess->LinkCnt--;

	/** Time to close the session down? **/
	if (sess->LinkCnt <= 0)
	    {
	    printf("NHT: releasing session for username [%s], cookie [%s]\n", sess->Username, sess->Cookie);

	    /** Decrement user session count **/
	    sess->User->SessionCnt--;
	    xaRemoveItem(&(sess->User->Sessions), xaFindItem(&(sess->User->Sessions), (void*)sess));

	    /** Remove the session from the global session list. **/
	    xhRemove(&(NHT.CookieSessions), sess->Cookie);
	    xhRemove(&(NHT.SessionsByID), sess->S_ID_Text);
	    xaRemoveItem(&(NHT.Sessions), xaFindItem(&(NHT.Sessions), (void*)sess));

	    /** Kill the inactivity timers now so we don't get re-entered **/
	    nht_i_RemoveWatchdog(sess->WatchdogTimer);
	    nht_i_RemoveWatchdog(sess->InactivityTimer);

	    /** Destroy any active app groups. **/
	    while(sess->AppGroups.nItems)
		{
		nht_i_UnlinkAppGroup((pNhtAppGroup)(sess->AppGroups.Items[0]));
		}

	    /** Close all open handles. **/
	    xhnClearHandles(&(sess->Hctx), nht_i_UnlinkSess_r);

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
	    /*while(sess->Triggers.nItems)
		{
		trg = (pNhtConnTrigger)(sess->Triggers.Items[0]);
		trg->LinkCnt--;
		if (trg->LinkCnt <= 0)
		    {
		    syDestroySem(trg->TriggerSem,SEM_U_HARDCLOSE);
		    xaRemoveItem(&(sess->Triggers), 0);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    }
		}*/

	    /** Clear control msg list **/
	    while(sess->ErrorList.nItems)
		{
		cm = (pNhtControlMsg)(sess->ControlMsgsList.Items[0]);
		xaRemoveItem(&sess->ControlMsgsList, 0);
		nht_i_FreeControlMsg(cm);
		}

	    /** Query data **/
	    for(i=0;i<sess->OsmlQueryList.nItems;i++)
		{
		nht_query = (pNhtQuery)(sess->OsmlQueryList.Items[i]);
		if (nht_query->ParamData) stFreeInf_ne(nht_query->ParamData);
		if (nht_query->ParamList) expFreeParamList(nht_query->ParamList);
		nmFree(nht_query, sizeof(NhtQuery));
		}

	    if (sess->Session)
		mssUnlinkSession(sess->Session);

	    /** Dealloc the xarrays and such **/
	    /*xaDeInit(&(sess->Triggers));*/
	    xaDeInit(&(sess->ErrorList));
	    xaDeInit(&(sess->ControlMsgsList));
	    xaDeInit(&(sess->OsmlQueryList));
	    xaDeInit(&(sess->AppGroups));
	    xhDeInit(sess->CachedApps);
	    nmFree(sess->CachedApps, sizeof(XHashTable));
	    xhnDeInitContext(&(sess->Hctx));
	    memset(sess, 0, sizeof(NhtSessionData));
	    nmFree(sess, sizeof(NhtSessionData));
	    }

    return 0;
    }


/*** nht_i_Watchdog() - manages the session watchdog timers.  These
 *** timers cause sessions to automatically time out if a certain operation
 *** isn't performed every so often.
 ***
 *** fyi - if the tick comparison arithmetic looks funny, it's because we
 *** need to use modulus arithmetic to prevent failure of this routine if
 *** (gasp!) Centrallix has been running for several hundred days (at
 *** CLK_TCK==100, the 32bit tick counter wraps at just under 500 days).
 ***/
void
nht_i_Watchdog(void* v)
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
		ev[0]->ReqLen = ((long long)(next_tick - cur_tick))*1000/NHT.ClkTck;
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


/*** nht_i_AddWatchdog() - adds a timer to the watchdog timer list.
 ***/
handle_t
nht_i_AddWatchdog(int timer_msec, int (*expire_fn)(), void* expire_arg)
    {
    pNhtTimer t;

	/** First, allocate ourselves a timer. **/
	t = (pNhtTimer)nmMalloc(sizeof(NhtTimer));
	if (!t) return XHN_INVALID_HANDLE;
	t->ExpireFn = expire_fn;
	t->ExpireArg = expire_arg;
	t->Time = timer_msec;
	t->ExpireTick = mtLastTick() + (((long long)timer_msec)*NHT.ClkTck/1000);
        t->Handle = xhnAllocHandle(&(NHT.TimerHctx), t);

	/** Add it to the list... **/
	syGetSem(NHT.TimerDataMutex, 1, 0);
	xaAddItem(&(NHT.Timers), (void*)t);
	syPostSem(NHT.TimerDataMutex, 1, 0);

	/** ... and post to the semaphore so the watchdog thread sees it **/
	syPostSem(NHT.TimerUpdateSem, 1, 0);

    return t->Handle;
    }


/*** nht_i_RemoveWatchdog() - removes a timer from the timer list.
 *** Returns 0 if it successfully removed it, or -1 if the timer did not
 *** exist.  In either case, the caller MUST consider the pointer 't'
 *** as invalid once the call returns.
 ***/
int
nht_i_RemoveWatchdog(handle_t th)
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


/*** nht_i_ResetWatchdog() - resets the countdown timer for a 
 *** given watchdog to its original value.  Returns 0 if successful,
 *** or -1 if the thing is no longer valid.
 ***/
int
nht_i_ResetWatchdog(handle_t th)
    {
    int idx;
    pNhtTimer t = xhnHandlePtr(&(NHT.TimerHctx), th);

	if (!t) return -1;

	/** Find the timer on the list, if it exists. **/
	syGetSem(NHT.TimerDataMutex, 1, 0);
	idx = xaFindItem(&(NHT.Timers), (void*)t);
	if (idx >= 0) t->ExpireTick = mtLastTick() + (((long long)t->Time)*NHT.ClkTck/1000);
	syPostSem(NHT.TimerDataMutex, 1, 0);

	/** ... and let the watchdog thread know about it **/
	syPostSem(NHT.TimerUpdateSem, 1, 0);

    return (idx >= 0)?0:-1;
    }


/*** nht_i_WatchdogTime() - returns the remaining time for a watchdog
 *** timer, in milliseconds.
 ***/
int
nht_i_WatchdogTime(handle_t th)
    {
    pNhtTimer t = xhnHandlePtr(&(NHT.TimerHctx), th);

	if (!t) return -1;

    return ((long long)(t->ExpireTick - mtLastTick())) * 1000 / NHT.ClkTck;
    }


/*** nht_i_WTimeout() - this routine is called when the session 
 *** watchdog timer expires for a session.  The response is to remove the
 *** session.
 ***/
int
nht_i_WTimeout(void* sess_v)
    {
    pNhtSessionData sess = (pNhtSessionData)sess_v;

	/** Disable the other timer and unlink from the session. **/
	nht_i_RemoveWatchdog(sess->InactivityTimer);
	nht_i_UnlinkSess(sess);

    return 0;
    }


/*** nht_i_ITimeout() - this routine is called when the session 
 *** inactivity timer expires for a session.  The response is to remove the
 *** session.
 ***/
int
nht_i_ITimeout(void* sess_v)
    {
    pNhtSessionData sess = (pNhtSessionData)sess_v;

	/** Disable the other timer and unlink from the session. **/
	nht_i_RemoveWatchdog(sess->WatchdogTimer);
	nht_i_UnlinkSess(sess);

    return 0;
    }


/*** nht_i_WTimeoutApp() - this routine is called when the session 
 *** watchdog timer expires for an application.  The response is to remove the
 *** application.
 ***/
int
nht_i_WTimeoutApp(void* app_v)
    {
    pNhtApp app = (pNhtApp)app_v;

	/** Free the App. **/
	nht_i_UnlinkApp(app);

    return 0;
    }


/*** nht_i_ITimeoutApp()
 ***/
int
nht_i_ITimeoutApp(void* app_v)
    {
    pNhtApp app = (pNhtApp)app_v;

	/** Right now, does the same as WTimeout, so call that. **/
	nht_i_WTimeoutApp(app);

    return 0;
    }


/*** nht_i_WTimeoutAppGroup() - this routine is called when the session 
 *** watchdog timer expires for an app group.  The response is to remove the
 *** application group.
 ***/
int
nht_i_WTimeoutAppGroup(void* group_v)
    {
    pNhtAppGroup group = (pNhtAppGroup)group_v;

	/** Free the App. **/
	nht_i_UnlinkAppGroup(group);

    return 0;
    }


/*** nht_i_ITimeoutAppGroup()
 ***/
int
nht_i_ITimeoutAppGroup(void* group_v)
    {
    pNhtAppGroup group = (pNhtAppGroup)group_v;

	/** Right now, does the same as WTimeout, so call that. **/
	nht_i_WTimeoutAppGroup(group);

    return 0;
    }


/*** nht_i_LogoutUser - Logout all sessions by a given user.
 ***/
int
nht_i_LogoutUser(char* username)
    {
    int i;
    pNhtSessionData search_s;
    pNhtUser usr;

	usr = (pNhtUser)xhLookup(&(NHT.UsersByName), username);
	if (!usr)
	    return -1;

	for(i=0;i<xaCount(&usr->Sessions);i++)
	    {
	    search_s = xaGetItem(&usr->Sessions, i);
	    search_s->Closed = 1;
	    nht_i_UnlinkSess(search_s);
	    }

    return 0;
    }


/*** nht_i_DiscardASession - search for sessions by a given user,
 *** and discard a suitably old one.
 ***/
int
nht_i_DiscardASession(pNhtUser usr)
    {
    int i;
    pNhtSessionData search_s, old_s = NULL;

	for(i=0;i<xaCount(&usr->Sessions);i++)
	    {
	    search_s = xaGetItem(&usr->Sessions, i);
	    if (!old_s || old_s->LastAccess > search_s->LastAccess)
		old_s = search_s;
	    }
	if (old_s)
	    nht_i_UnlinkSess(old_s);

    return 0;
    }


/*** nht_i_AllocApp() - allocate a new application structure
 ***/
pNhtApp
nht_i_AllocApp(char* path, pNhtAppGroup group)
    {
    pNhtApp app = NULL;
    int akey[2];
    char akeybuf[256];

	/** Allocate memory **/
	app = (pNhtApp)nmMalloc(sizeof(NhtApp));
	if (!app) return NULL;
	memset(app, 0, sizeof(NhtApp));

	/** Reference count:
	 ** - one for the caller (connection)
	 ** - one for the watchdog manager (session persistence)
	 **/
	app->LinkCnt = 2;
	cxssGenerateKey((unsigned char*)akey, sizeof(akey));
	sprintf(app->AKey, "%8.8x%8.8x", akey[0], akey[1]);
	snprintf(akeybuf, sizeof(akeybuf), "%s-%s-%s", group->Session->SKey, group->GKey, app->AKey);
	app->Application = appCreate(akeybuf);
	if (!app->Application)
	    goto error;
	strtcpy(app->AppPathname, path, sizeof(app->AppPathname));
	app->A_ID = NHT.A_ID_Count++;
	app->Group = group;
	snprintf(app->A_ID_Text, sizeof(app->A_ID_Text), "%s|%lld", group->G_ID_Text, app->A_ID);
	objCurrentDate(&(app->FirstActivity));
	objCurrentDate(&(app->LastActivity));
	app->AppObjSess = objOpenSession("/");
	if (!app->AppObjSess)
	    goto error;
	app->WatchdogTimer = nht_i_AddWatchdog(NHT.WatchdogTime*1000, nht_i_WTimeoutApp, (void*)app);
	if (app->WatchdogTimer == XHN_INVALID_HANDLE)
	    goto error;
	/*app->InactivityTimer = nht_i_AddWatchdog(NHT.InactivityTime*1000, nht_i_ITimeoutApp, (void*)app);*/
	xaAddItem(&group->Apps, (void*)app);
	xaInit(&app->Endorsements, 16);
	xaInit(&app->Contexts, 16);
	xaInit(&app->AppOSMLSessions, 16);

	return app;

    error:
	if (app)
	    {
	    if (app->AppObjSess)
		objCloseSession(app->AppObjSess);
	    if (app->Application)
		appDestroy(app->Application);
	    nmFree(app, sizeof(NhtApp));
	    }
	return NULL;
    }


/*** nht_i_LinkApp() - free and clean up an application structure
 ***/
pNhtApp
nht_i_LinkApp(pNhtApp app)
    {

	app->LinkCnt += 1;

    return app;
    }


/*** nht_i_UnlinkApp() - free and clean up an application structure
 ***/
int
nht_i_UnlinkApp(pNhtApp app)
    {
    int i;
    pObjSession one_sess;

	/** Reference check **/
	app->LinkCnt -= 1;
	if (app->LinkCnt > 0)
	    return 0;

	printf("NHT: Releasing app %s: %s\n", app->AKey, app->AppPathname);

	/** Disconnect from the app group **/
	xaRemoveItem(&(app->Group->Apps), xaFindItem(&(app->Group->Apps), (void*)app));

	/** Remove watchdog to avoid further callbacks **/
	nht_i_RemoveWatchdog(app->WatchdogTimer);

	/** Free the endorsement/context info **/
	for(i=0;i<app->Endorsements.nItems;i++)
	    {
	    nmSysFree(app->Endorsements.Items[i]);
	    nmSysFree(app->Contexts.Items[i]);
	    }
	xaDeInit(&app->Endorsements);
	xaDeInit(&app->Contexts);

	/** Close OSML sessions we opened during this particular app **/
	for(i=0; i<xaCount(&app->AppOSMLSessions); i++)
	    {
	    one_sess = (pObjSession)xaGetItem(&app->AppOSMLSessions, i);
	    xhnFreeHandle(&app->Group->Session->Hctx, xhnHandle(&app->Group->Session->Hctx, one_sess));
	    objCloseSession(one_sess);
	    }
	xaDeInit(&app->AppOSMLSessions);

	/** Clean up... **/
	objCloseSession(app->AppObjSess);
	appDestroy(app->Application);
	nmFree(app, sizeof(NhtApp));

    return 0;
    }


/*** nht_i_AllocAppGroup() - allocate a new application group structure
 ***/
pNhtAppGroup
nht_i_AllocAppGroup(char* path, pNhtSessionData s)
    {
    pNhtAppGroup group;
    int gkey[2];

	/** Allocate memory **/
	group = (pNhtAppGroup)nmMalloc(sizeof(NhtAppGroup));
	if (!group) return NULL;
	memset(group, 0, sizeof(NhtAppGroup));

	/** Reference count:
	 ** - one for the caller (connection)
	 ** - one for the watchdog manager (session persistence)
	 **/
	group->LinkCnt = 2;
	strtcpy(group->StartURL, path, sizeof(group->StartURL));
	group->G_ID = NHT.G_ID_Count++;
	cxssGenerateKey((unsigned char*)gkey, sizeof(gkey));
	sprintf(group->GKey, "%8.8x%8.8x", gkey[0], gkey[1]);
	group->Session = s;
	snprintf(group->G_ID_Text, sizeof(group->G_ID_Text), "%s|%lld", s->S_ID_Text, group->G_ID);
	objCurrentDate(&(group->FirstActivity));
	objCurrentDate(&(group->LastActivity));
	group->WatchdogTimer = nht_i_AddWatchdog(NHT.WatchdogTime*1000, nht_i_WTimeoutAppGroup, (void*)group);
	/*group->InactivityTimer = nht_i_AddWatchdog(NHT.InactivityTime*1000, nht_i_ITimeoutAppGroup, (void*)group);*/
	xaInit(&group->Apps, 16);
	xaAddItem(&s->AppGroups, (void*)group);

    return group;
    }


/*** nht_i_LinkAppGroup() - free and clean up an application structure
 ***/
pNhtAppGroup
nht_i_LinkAppGroup(pNhtAppGroup group)
    {

	group->LinkCnt += 1;

    return group;
    }


/*** nht_i_UnlinkAppGroup() - free and clean up an application structure
 ***/
int
nht_i_UnlinkAppGroup(pNhtAppGroup group)
    {

	/** Reference check **/
	group->LinkCnt -= 1;
	if (group->LinkCnt > 0)
	    return 0;

	printf("NHT: Releasing app group %s: %s\n", group->GKey, group->StartURL);

	/** Disconnect from the session **/
	xaRemoveItem(&(group->Session->AppGroups), xaFindItem(&(group->Session->AppGroups), (void*)group));
	
	nht_i_RemoveWatchdog(group->WatchdogTimer);

	/** Destroy any apps still active **/
	while(group->Apps.nItems)
	    {
	    nht_i_UnlinkApp((pNhtApp)(group->Apps.Items[0]));
	    }

	/** Clean up... **/
	xaDeInit(&group->Apps);
	nmFree(group, sizeof(NhtAppGroup));

    return 0;
    }


/*** nht_i_VerifyAKey() - given an akey (CSRF token) from the client,
 *** verify that it matches the key in the session, and if the group and app
 *** parts match an existing app group and app, then return the group and app
 *** to the caller, as well.
 ***
 *** Key formats:
 ***
 ***    session only - "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS"
 ***    session and group - "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS-GGGGGGGGGGGGGGGG"
 ***    session, group, and app - "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS-GGGGGGGGGGGGGGGG-AAAAAAAAAAAAAAAA"
 ***/
int
nht_i_VerifyAKey(char* client_key, pNhtSessionData sess, pNhtAppGroup *group, pNhtApp *app)
    {
    int i;
    pNhtAppGroup group_search;
    pNhtAppGroup found_group = NULL;
    pNhtApp app_search;

	if (group) *group = NULL;
	if (app) *app = NULL;

	/** Session key matches? **/
	if (strlen(client_key) < 32 || strncmp(client_key, sess->SKey, strlen(sess->SKey)) != 0)
	    {
	    return -1;
	    }

	/** Now check for a valid group key **/
	if (strlen(client_key) >= 49)
	    {
	    for(i=0;i<sess->AppGroups.nItems;i++)
		{
		group_search = (pNhtAppGroup)(sess->AppGroups.Items[i]);
		if (!strncmp(client_key+33, group_search->GKey, strlen(group_search->GKey)))
		    {
		    found_group = group_search;
		    if (group) *group = group_search;
		    break;
		    }
		}

	    /** ... and now for a valid application key too **/
	    if (found_group && strlen(client_key) >= 66)
		{
		for(i=0;i<found_group->Apps.nItems;i++)
		    {
		    app_search = (pNhtApp)(found_group->Apps.Items[i]);
		    if (!strncmp(client_key+50, app_search->AKey, strlen(app_search->AKey)))
			{
			if (app) *app = app_search;
			break;
			}
		    }
		}
	    }

    return 0;
    }


/*** nht_i_LookupApp() - given an akey, lookup the session, group, and
 *** app that the key is associated with.
 ***/
int
nht_i_LookupApp(char* akey, pNhtSessionData *sess, pNhtAppGroup *group, pNhtApp *app)
    {
    pNhtSessionData sess_search;
    int i;

	if (sess) *sess = NULL;
	if (group) *group = NULL;
	if (app) *app = NULL;

	/** Too short? **/
	if (!akey || strlen(akey) < 32)
	    return -1;

	/** Search for the session **/
	for(i=0; i<NHT.Sessions.nItems; i++)
	    {
	    sess_search = (pNhtSessionData)NHT.Sessions.Items[i];
	    if (sess_search && strncmp(akey, sess_search->SKey, strlen(sess_search->SKey)) == 0)
		{
		if (sess) *sess = sess_search;
		return nht_i_VerifyAKey(akey, sess_search, group, app);
		}
	    }

    return -1;
    }

