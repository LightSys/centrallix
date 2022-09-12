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
#include "cxlib/memstr.h"
#include "cxlib/xstring.h"
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
#include "json/json.h"
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


 #define DEBUG_OSML	0

/*** payload limitation ***/
#define	NHT_PAYLOAD_MAX		(1024*1024*4)	/* 4 megabytes */

/*** one HTTP header ***/
typedef struct
    {
    char	Name[64];
    char*	Value;
    int		ValueAlloc:1;
    }
    HttpHeader, *pHttpHeader;


/*** This structure is used for wait-for-conn-to-finish ***/
/*typedef struct
    {
    int		TriggerID;
    pSemaphore	TriggerSem;
    int		LinkCnt;
    }
    NhtConnTrigger, *pNhtConnTrigger;*/

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
    DateTime		FirstActivity;
    DateTime		LastActivity;
    XArray		Sessions;
    }
    NhtUser, *pNhtUser;


/*** This is used to keep track of user/password/cookie information ***/
typedef struct
    {
    char	Username[32];
    char	Password[32];
    char	Cookie[64];
    char	SKey[64];
    long long	S_ID;		/* incrementing session id counter */
    char	S_ID_Text[24];
    char	HTTPVer[16];
    int		ver_10:1;	/* is HTTP/1.0 compatible */
    int		ver_11:1;	/* is HTTP/1.1 compatible */
    void*	Session;
    int		IsNewCookie;
    MTSecContext SecurityContext;
    pObjSession	ObjSess;
    pSemaphore	Errors;
    XArray	ErrorList;	/* xarray of xstring */
/*    XArray	Triggers;	** xarray of pNhtConnTrigger */
    HandleContext Hctx;
    DateTime	FirstActivity;
    DateTime	LastActivity;
    handle_t	WatchdogTimer;
    handle_t	InactivityTimer;
    int		LinkCnt;
    pSemaphore	ControlMsgs;
    XArray	ControlMsgsList;
    pNhtUser	User;
    int		LastAccess;
    pXHashTable	CachedApps;
    char	LastIPAddr[20];
    XArray	OsmlQueryList;	/* array of pNhtQuery */
    XArray	AppGroups;	/* array of pNhtAppGroup */
    }
    NhtSessionData, *pNhtSessionData;


/*** Application group.  Each time the user starts a new .app "from scratch"
 *** (i.e., without passing in another app handle), a new application group
 *** structure is created with a new key.
 ***/
typedef struct
    {
    char	GKey[64];	/* random application group identifier */
    long long	G_ID;		/* incrementing app group id counter */
    char	G_ID_Text[24+24];
    char	StartURL[OBJSYS_MAX_PATH + 1];
    DateTime	FirstActivity;
    DateTime	LastActivity;
    handle_t	WatchdogTimer;
    /*handle_t	InactivityTimer;*/
    pNhtSessionData Session;
    XArray	Apps;		/* array of pNhtApp */
    int		LinkCnt;
    }
    NhtAppGroup, *pNhtAppGroup;

/*** Post payload.  One of these is created for each file uploaded through Post(using
 *** the file upload widget).
 ***/
typedef struct
    {
    char    filename[128];
    char    newname[512];
    char    path[512];
    char    full_new_path[1024]; //path + newname
    char    extension[16];
    char    mime_type[128];
    int     status;
    }
    NhtPostPayload, *pNhtPostPayload;


/*** One-page app data.  Each time the user launches an .app, a new app
 *** structure is created with a new key.
 ***/
typedef struct
    {
    char	AKey[64];	/* random app ID */
    long long	A_ID;		/* incrementing app id counter */
    char	A_ID_Text[24+24+24];
    char	AppPathname[OBJSYS_MAX_PATH + 1];
    DateTime	FirstActivity;
    DateTime	LastActivity;
    handle_t	WatchdogTimer;
    /*handle_t	InactivityTimer;*/
    pObjSession	AppObjSess;
    pNhtAppGroup    Group;
    XArray	AppOSMLSessions;
    XArray	Endorsements;
    XArray	Contexts;
    pApplication Application;
    int		LinkCnt;
    }
    NhtApp, *pNhtApp;


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


/*** Timer structure.  The rule on the deallocation of these is that if
 *** you successfully remove it from the Timers list while holding the
 *** TimerDataMutex, then the thing is yours to deallocate and no one
 *** else should be messing with it.
 ***/
typedef struct
    {
    unsigned int	Time;		/* # msec this goes for */
    int			(*ExpireFn)();
    void*		ExpireArg;
    unsigned int	ExpireTick;	/* internal expiration time mark */
    int			SeqID;
    handle_t		Handle;
    }
    NhtTimer, *pNhtTimer;


/*** Connection data ***/
typedef struct
    {
    pFile	ConnFD;
    pStruct	ReqURL;
    pNhtSessionData	NhtSession;
    pNhtAppGroup	AppGroup;
    pNhtApp		App;
    int		InBody;
    int		BytesWritten;
    int		ResultCode;
    int		Port;
    char*	URL;
    char	AcceptEncoding[160];
    char*	Referrer;
    char	UserAgent[160];
    char	RequestContentType[128];
    char	RequestBoundary[128];
    char	Method[16];
    char	HTTPVer[16];
    char	Cookie[160];
    char	AllCookies[640];
    char	Auth[160];
    char	Destination[256];
    char	IfModifiedSince[64];
    char	Username[32];
    char	Password[32];
    char	IPAddr[20];
    int		Size;
    int		NotActivity;
    handle_t	LastHandle;
    pFile	ReportingFD;
    int		SSLpid;
    int		Keepalive:1;
    int		NoCache:1;
    int		UsingTLS:1;
    int		UsingChunkedEncoding:1;
    int		StrictSameSite:1;
    char	ResponseContentType[128];
    int		ResponseContentLength;
    XArray	RequestHeaders;		/* of pHttpHeader */
    XArray	ResponseHeaders;	/* of pHttpHeader */
    int		ResponseCode;
    char	ResponseText[64];
    char	ResponseTime[48];
    }
    NhtConn, *pNhtConn;


/*** GLOBALS ***/
typedef struct 
    {
    XHashTable	CookieSessions;
    XHashTable	SessionsByID;
    XArray	Sessions;
    char	ServerString[80];
    char	Realm[80];
    char	SessionCookie[32];
    char	TlsSessionCookie[32];
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
    long long	S_ID_Count;
    long long	G_ID_Count;
    long long	A_ID_Count;
    SSL_CTX*	SSL_ctx;
    char	UploadTmpDir[256];	/* default is /var/tmp */
    XArray	AllowedUploadDirs;	/* where uploads can eventually go */
    XArray	AllowedUploadExts;	/* allowed extensions */
    pSemaphore	CollectedConns;
    pSemaphore	CollectedTLSConns;
    pCxssKeystreamState NonceData;
    unsigned char   LoginKey[32];	/* 256-bit hash secret for Basic auth logins */
    int		AuthMethods;		/* allowed authentication methods */
    }
    NHT_t;

extern NHT_t NHT;

#define NHT_XFO_T_NONE		0
#define NHT_XFO_T_DENY		1
#define NHT_XFO_T_SAMEORIGIN	2

#define NHT_AUTH_HTTP		1	/* HTTP Basic auth */
#define NHT_AUTH_HTTPSTRICT	2	/* HTTP Basic auth with strict logout management */
#define NHT_AUTH_HTTPBEARER	4	/* HTTP Bearer token auth (future) */
#define NHT_AUTH_WEBFORM	8	/* Web form based auth (future) */

typedef struct
    {
    int Key;
    pWgtrNode Node;
    }
    CachedApp, *pCachedApp;

pCachedApp CachedAppConstructor();
int CachedAppInit(pCachedApp this);

pNhtSessionData nht_i_AllocSession(char* usrname, int using_tls);
handle_t nht_i_AddWatchdog(int timer_msec, int (*expire_fn)(), void* expire_arg);
int nht_i_RemoveWatchdog(handle_t th);
void nht_i_Watchdog(void* v);
int nht_i_WatchdogTime(handle_t th);

int nht_i_VerifyAKey(char* client_key, pNhtSessionData sess, pNhtAppGroup *group, pNhtApp *app);

pNhtApp nht_i_AllocApp(char* path, pNhtAppGroup g);
pNhtApp nht_i_LinkApp(pNhtApp app);
int nht_i_UnlinkApp(pNhtApp app);
pNhtAppGroup nht_i_AllocAppGroup(char* path, pNhtSessionData s);
pNhtAppGroup nht_i_LinkAppGroup(pNhtAppGroup group);
int nht_i_UnlinkAppGroup(pNhtAppGroup group);

void nht_i_TLSHandler(void* v);
void nht_i_Handler(void* v);
int nht_i_ITimeout(void* sess_v);
int nht_i_WTimeout(void* sess_v);
int nht_i_ITimeoutAppGroup(void* sess_v);
int nht_i_WTimeoutAppGroup(void* sess_v);
int nht_i_ITimeoutApp(void* sess_v);
int nht_i_WTimeoutApp(void* sess_v);

/*int nht_i_WriteResponse(pNhtConn conn, int code, char* text, int contentlen, char* contenttype, char* pragma, char* resptxt);*/
int nht_i_WriteResponse(pNhtConn conn, int code, char* text, char* resptxt);
void nht_i_ErrorExit(pNhtConn conn, int code, char* text) __attribute__ ((noreturn));
int nht_i_AddHeader(pXArray hdrlist, char* hdrname, char* hdrval, int hdralloc);
int nht_i_AddResponseHeader(pNhtConn conn, char* hdrname, char* hdrval, int hdralloc);
int nht_i_AddResponseHeaderQPrintf(pNhtConn conn, char* hdrname, char* hdrfmt, ...);
char* nht_i_GetHeader(pXArray hdrlist, char* hdrname);
int nht_i_FreeHeaders(pXArray hdrlist);
int nht_i_CheckAccessLog();

/*** REST implementation ***/
int nht_i_RestGet(pNhtConn conn, pStruct url_inf, pObject obj);
int nht_i_RestPatch(pNhtConn conn, pStruct url_inf, pObject obj, struct json_object*);
int nht_i_RestPost(pNhtConn conn, pStruct url_inf, int size, char* content);
int nht_i_RestDelete(pNhtConn conn, pStruct url_inf, pObject obj);

#endif
