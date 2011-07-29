#ifndef _NET_HTTP_H
#define _NET_HTTP_H

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h> //for regex functions
#include <regex.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_LIBZ 1
#endif

#include "centrallix.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/exception.h"
#include "obj.h"
#include "stparse_ne.h"
#include "stparse.h"
#include "htmlparse.h"
#include "cxlib/xhandle.h"
#include "cxlib/magic.h"
#include "cxlib/util.h"
#include "wgtr.h"
#include "iface.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
#include "cxss/cxss.h"

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

/**CVSDATA***************************************************************

    $Id: net_http.h,v 1.4 2010/09/17 15:45:29 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_http.h,v $

    $Log: net_http.h,v $
    Revision 1.4  2010/09/17 15:45:29  gbeeley
    - (security) implement X-Frame-Options anti-clickjacking countermeasure

    Revision 1.3  2009/06/26 18:31:03  gbeeley
    - (feature) enhance ls__method=copy so that it supports srctype/dsttype
      like test_obj does
    - (feature) add ls__rowcount row limiter to sql query mode (non-osml)
    - (change) some refactoring of error message handlers to clean things
      up a bit
    - (feature) adding last_activity to session objects (for sysinfo)
    - (feature) parameterized OSML SQL queries over the http interface

    Revision 1.2  2008/08/16 00:31:38  thr4wn
    I made some more modification of documentation and begun logic for
    caching generated WgtrNode instances (see centrallix-sysdoc/misc.txt)

    Revision 1.1  2008/06/25 22:48:12  jncraton
    - (change) split net_http into separate files
    - (change) replaced nht_internal_UnConvertChar with qprintf filter
    - (change) replaced nht_internal_escape with qprintf filter
    - (change) replaced nht_internal_decode64 with qprintf filter
    - (change) removed nht_internal_Encode64
    - (change) removed nht_internal_EncodeHTML


 **END-CVSDATA***********************************************************/

 #define DEBUG_OSML	0

/*** This structure is used for wait-for-conn-to-finish ***/
typedef struct
    {
    int		TriggerID;
    pSemaphore	TriggerSem;
    int		LinkCnt;
    }
    NhtConnTrigger, *pNhtConnTrigger;

/*** Parameter/information for a control message, as embedded within a
 *** link on the page.
 ***/
typedef struct
    {
    char*	P1;		/* param #1: link TARGET */
    char*	P2;		/* param #2: link TXT */
    char*	P3;		/* param #3: link HREF, if entire HREF. */
    char*	P3a;		/* param #3a: link SERVER if partitioned HREF. */
    char*	P3b;		/* param #3b: link PATH if partitioned HREF. */
    char*	P3c;		/* param #3c: link QUERY if partitioned HREF. */
    char*	P3d;		/* param #3d: link JUMP-TARGET if partitioned HREF. */
    }
    NhtControlMsgParam, *pNhtControlMsgParam;

/*** This structure is used for server-to-client OOB control messages. ***/
typedef struct _NCM
    {
    int		MsgType;	/* NHT_CONTROL_T_xxx */
    XArray	Params;		/* xarray of pNhtControlMsgParam */
    pSemaphore	ResponseSem;	/* if set, control msg posts here when user responds */
    int		(*ResponseFn)(); /* if set, control msg calls this fn when user responds */
    int		Status;		/* status - NHT_CONTROL_S_xxx */
    char*	Response;	/* response string received from client */
    void*	Context;	/* caller-defined */
    }
    NhtControlMsg, *pNhtControlMsg;

#define NHT_CONTROL_T_ERROR	1	/* error message */
#define NHT_CONTROL_T_QUERY	2	/* query user for information */
#define NHT_CONTROL_T_GOODBYE	4	/* server shutting down */
#define NHT_CONTROL_T_REPMSG	8	/* replication message */
#define NHT_CONTROL_T_EVENT	16	/* remote control channel event */

#define NHT_CONTROL_S_QUEUED	0	/* message queued waiting for client */
#define NHT_CONTROL_S_SENT	1	/* message sent to client */
#define NHT_CONTROL_S_ERROR	2	/* could not get client's response */
#define NHT_CONTROL_S_RESPONSE	3	/* client has responded to message */

/*** User structure
 ***/
typedef struct
    {
    char		Username[32];
    int			SessionCnt;
    DateTime		LastActivity;
    }
    NhtUser, *pNhtUser;


/*** This is used to keep track of user/password/cookie information ***/
typedef struct
    {
    char	Username[32];
    char	Password[32];
    char	Cookie[64];
    char	AKey[64];
    char	HTTPVer[16];
    int		ver_10:1;	/* is HTTP/1.0 compatible */
    int		ver_11:1;	/* is HTTP/1.1 compatible */
    void*	Session;
    int		IsNewCookie;
    MTSecContext SecurityContext;
    pObjSession	ObjSess;
    pSemaphore	Errors;
    XArray	ErrorList;	/* xarray of xstring */
    XArray	Triggers;	/* xarray of pNhtConnTrigger */
    HandleContext Hctx;
    HandleContext HctxUp;
    handle_t	WatchdogTimer;
    handle_t	InactivityTimer;
    int		LinkCnt;
    pSemaphore	ControlMsgs;
    XArray	ControlMsgsList;
    pNhtUser	User;
    int		LastAccess;
    pXHashTable	CachedApps;
    XArray	OsmlQueryList;	/* array of pNhtQuery */
    }
    NhtSessionData, *pNhtSessionData;


/*** Query structure.  Used for storing information about an open query.  These
 *** are stored in the OsmlQueryList in the NhtSessionData structure.
 ***/
typedef struct
    {
    pObjQuery	OsmlQuery;	/* osml query handle */
    handle_t	QueryHandle;
    int		FetchCount;
    pStruct	ParamData;	/* parameter data from client */
    pParamObjects ParamList;	/* parameter list, to represent ParamData */
    }
    NhtQuery, *pNhtQuery;

/**
 * @brief Replication to client update request
 */
typedef struct
    {
    ///@brief Internal magic number
    int Magic;
    ///@brief Saved queries
    pXTree Querys;
    ///@brief Last know results of a query
    pXHashTable Saved;
    ///@brief Update notification requests we have open
    pXHashTable Notifications;
    ///@brief Update notification names (for iteration)
    pXArray NotificationNames;
    }
    NhtUpdate, *pNhtUpdate;

/*** Timer structure.  The rule on the deallocation of these is that if
 *** you successfully remove it from the Timers list while holding the
 *** TimerDataMutex, then the thing is yours to deallocate and no one
 *** else should be messing with it.
 ***/
typedef struct
    {
    unsigned long	Time;		/* # msec this goes for */
    int			(*ExpireFn)();
    void*		ExpireArg;
    unsigned long	ExpireTick;	/* internal expiration time mark */
    int			SeqID;
    handle_t		Handle;
    }
    NhtTimer, *pNhtTimer;


/*** Connection data ***/
typedef struct
    {
    pFile	ConnFD;
    pStruct	ReqURL;
    pNhtSessionData NhtSession;
    int		InBody;
    int		BytesWritten;
    int		ResultCode;
    int		Port;
    char*	URL;
    char	AcceptEncoding[160];
    char*	Referrer;
    char	UserAgent[160];
    char	RequestContentType[64];
    char	RequestBoundary[128];
    char	Method[16];
    char	HTTPVer[16];
    char	Cookie[160];
    char	Auth[160];
    char	Destination[256];
    char	IfModifiedSince[64];
    char	Username[32];
    char	Password[32];
    char	IPAddr[20];
    int		Size;
    handle_t	LastHandle;
    }
    NhtConn, *pNhtConn;


/*** GLOBALS ***/
struct 
    {
    XHashTable	CookieSessions;
    XArray	Sessions;
    char	ServerString[80];
    char	Realm[80];
    char	SessionCookie[32];
    pSemaphore	TimerUpdateSem;
    pSemaphore	TimerDataMutex;
    XArray	Timers;
    HandleContext TimerHctx;
    int		WatchdogTime;
    int		InactivityTime;
    regex_t*	reNet47;
    int		EnableGzip;
    int		CondenseJS;
    int		RestrictToLocalhost;
    char*	DirIndex[16];
    int		UserSessionLimit;
    XHashTable	UsersByName;
    XArray	UsersList;
    long long	AccCnt;
    pFile	AccessLogFD;
    char	AccessLogFile[256];
    int		ClkTck;
    int		numbCachedApps;
    int		XFrameOptions;		/* NHT_XFO_T_xxx */
    }
    NHT;

#define NHT_XFO_T_NONE		0
#define NHT_XFO_T_DENY		1
#define NHT_XFO_T_SAMEORIGIN	2

typedef struct
    {
    int Key;
    pWgtrNode Node;
    }
    CachedApp, *pCachedApp;

pCachedApp CachedAppConstructor();
int CachedAppInit(pCachedApp this);

int nht_internal_RemoveWatchdog(handle_t th);
void nht_internal_Watchdog(void* v);

void nht_internal_Handler(void* v);
int nht_internal_ITimeout(void* sess_v);
int nht_internal_WTimeout(void* sess_v);

int nht_internal_WriteResponse(pNhtConn conn, int code, char* text, int contentlen, char* contenttype, char* pragma, char* resptxt);
void nht_internal_ErrorExit(pNhtConn conn, int code, char* text);
int nht_internal_GetUpdates(pNhtConn conn,pStruct url_inf);
pNhtUpdate nht_internal_createUpdates();
void nht_internal_freeUpdates(pNhtUpdate update);
 #endif
